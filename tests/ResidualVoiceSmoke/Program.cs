using System;
using System.Runtime.InteropServices;
using System.Text;

internal static class Program
{
    private const int MaxPacketSize = 1500;
    private const int MaxMessageSize = 1024;

    private static void Main()
    {
        Console.WriteLine($"Native API version: {Native.rv_voice_get_api_version()}");

        var playerOne = CreateClient(playerId: 1);
        var playerTwo = CreateClient(playerId: 2);

        try
        {
            var frameSamples = Native.rv_voice_get_required_frame_samples(playerOne);

            if (frameSamples == 0)
            {
                throw new Exception("rv_voice_get_required_frame_samples returned 0.");
            }

            Console.WriteLine($"Frame samples: {frameSamples}");

            Connect(playerOne, sessionId: 12345, playerId: 1);
            Connect(playerTwo, sessionId: 12345, playerId: 2);

            DrainEvents(playerOne, "P1", expectPcm: false);
            DrainEvents(playerTwo, "P2", expectPcm: false);

            // Connect emits join packets. Drain those so the next P1 packet should be voice audio.
            DrainOutgoingPackets(playerOne, "P1", discardOnly: true);
            DrainOutgoingPackets(playerTwo, "P2", discardOnly: true);

            var pcm = BuildTestTone((int)frameSamples, sampleRate: 48000);

            ThrowIfError(
                Native.rv_voice_submit_capture_pcm(playerOne, pcm, (uint)pcm.Length),
                "rv_voice_submit_capture_pcm");

            ThrowIfError(Native.rv_voice_tick(playerOne, 20), "rv_voice_tick P1");

            var outgoingVoicePacket = PollOneOutgoingPacket(playerOne);

            if (outgoingVoicePacket.Length == 0)
            {
                throw new Exception("No outgoing voice packet was produced by Player 1.");
            }

            Console.WriteLine($"P1 outgoing voice packet bytes: {outgoingVoicePacket.Length}");

            ThrowIfError(
                Native.rv_voice_ingest_packet(
                    playerTwo,
                    outgoingVoicePacket,
                    (uint)outgoingVoicePacket.Length,
                    40),
                "rv_voice_ingest_packet P2");

            var gotPcm = false;

            for (var i = 0; i < 30 && !gotPcm; i++)
            {
                var nowMs = (uint)(60 + i * 20);

                ThrowIfError(Native.rv_voice_tick(playerTwo, nowMs), "rv_voice_tick P2");

                gotPcm = DrainEvents(playerTwo, "P2", expectPcm: true);
            }

            if (!gotPcm)
            {
                throw new Exception("Player 2 ingested the voice packet, but no PCM frame event was produced.");
            }

            Console.WriteLine("Packet loopback smoke test passed.");
        }
        finally
        {
            Native.rv_voice_destroy(playerTwo);
            Native.rv_voice_destroy(playerOne);
        }
    }

    private static IntPtr CreateClient(ushort playerId)
    {
        var config = new RvVoiceConfig
        {
            api_version = Native.rv_voice_get_api_version(),
            sample_rate_hz = 48000,
            frame_ms = 20,
            max_players = 16,
            jitter_target_ms = 60,
            jitter_max_ms = 200,
            capture_mode = RvVoiceCaptureMode.AlwaysOn,
            reserved_u32 = new uint[8]
        };

        var handle = Native.rv_voice_create(ref config, IntPtr.Zero, IntPtr.Zero);

        if (handle == IntPtr.Zero)
        {
            throw new Exception($"rv_voice_create returned null for player {playerId}.");
        }

        return handle;
    }

    private static void Connect(IntPtr handle, ulong sessionId, ushort playerId)
    {
        var info = new RvVoiceConnectInfo
        {
            session_id = sessionId,
            player_id = playerId,
            relay_host = IntPtr.Zero,
            relay_port = 0,
            local_port = 0,
            token = IntPtr.Zero
        };

        ThrowIfError(Native.rv_voice_connect(handle, ref info), $"rv_voice_connect player {playerId}");
    }

    private static short[] BuildTestTone(int sampleCount, int sampleRate)
    {
        var samples = new short[sampleCount];

        const double frequency = 440.0;
        const double amplitude = 0.20;

        for (var i = 0; i < samples.Length; i++)
        {
            var t = i / (double)sampleRate;
            samples[i] = (short)(Math.Sin(2.0 * Math.PI * frequency * t) * short.MaxValue * amplitude);
        }

        return samples;
    }

    private static byte[] PollOneOutgoingPacket(IntPtr handle)
    {
        var buffer = new byte[MaxPacketSize];

        var result = Native.rv_voice_poll_outgoing(
            handle,
            buffer,
            (uint)buffer.Length,
            out var outSize);

        if (result < 0)
        {
            ThrowIfError(result, "rv_voice_poll_outgoing");
        }

        if (result == 0)
        {
            return Array.Empty<byte>();
        }

        if (outSize == 0)
        {
            return Array.Empty<byte>();
        }

        var packet = new byte[outSize];
        Buffer.BlockCopy(buffer, 0, packet, 0, (int)outSize);

        return packet;
    }

    private static void DrainOutgoingPackets(IntPtr handle, string label, bool discardOnly)
    {
        while (true)
        {
            var packet = PollOneOutgoingPacket(handle);

            if (packet.Length == 0)
            {
                return;
            }

            if (!discardOnly)
            {
                Console.WriteLine($"{label} drained outgoing packet: {packet.Length} bytes");
            }
        }
    }

    private static bool DrainEvents(IntPtr handle, string label, bool expectPcm)
    {
        var gotPcm = false;
        var pcmBuffer = new short[2880];
        var messageBuffer = new byte[MaxMessageSize];

        while (true)
        {
            Array.Clear(messageBuffer, 0, messageBuffer.Length);

            var result = Native.rv_voice_poll_event_flat(
                handle,
                out var ev,
                pcmBuffer,
                (uint)pcmBuffer.Length,
                messageBuffer,
                (uint)messageBuffer.Length);

            if (result == 0)
            {
                return gotPcm;
            }

            if (result < 0)
            {
                ThrowIfError(result, "rv_voice_poll_event_flat");
            }

            var eventType = (RvVoiceEventType)ev.type;

            switch (eventType)
            {
                case RvVoiceEventType.Log:
                    Console.WriteLine($"{label} LOG: {ReadMessage(messageBuffer, ev.message_size)}");
                    break;

                case RvVoiceEventType.Connected:
                    Console.WriteLine($"{label} EVENT: Connected");
                    break;

                case RvVoiceEventType.Disconnected:
                    Console.WriteLine($"{label} EVENT: Disconnected");
                    break;

                case RvVoiceEventType.Speaking:
                    Console.WriteLine($"{label} EVENT: Speaking speaker={ev.speaker_id} speaking={ev.is_speaking}");
                    break;

                case RvVoiceEventType.PcmFrame:
                    Console.WriteLine(
                        $"{label} EVENT: PCM speaker={ev.speaker_id} samples={ev.sample_count} rate={ev.sample_rate} channels={ev.channels}");

                    gotPcm = true;
                    break;

                case RvVoiceEventType.Error:
                    Console.WriteLine($"{label} ERROR EVENT: {ReadMessage(messageBuffer, ev.message_size)}");
                    break;
            }

            if (expectPcm && gotPcm)
            {
                return true;
            }
        }
    }

    private static string ReadMessage(byte[] buffer, uint messageSize)
    {
        var count = Math.Min((int)messageSize, buffer.Length);
        return Encoding.UTF8.GetString(buffer, 0, count).TrimEnd('\0');
    }

    private static void ThrowIfError(int result, string callName)
    {
        if (result >= 0)
        {
            return;
        }

        var ptr = Native.rv_voice_result_to_string(result);
        var nativeMessage = ptr == IntPtr.Zero
            ? $"code {result}"
            : Marshal.PtrToStringAnsi(ptr) ?? $"code {result}";

        throw new InvalidOperationException($"{callName} failed: {nativeMessage}");
    }
}

internal static class Native
{
    private const string Lib = "residual_voice";

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern uint rv_voice_get_api_version();

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr rv_voice_create(
        ref RvVoiceConfig config,
        IntPtr allocators,
        IntPtr callbacks);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void rv_voice_destroy(IntPtr voice);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int rv_voice_connect(
        IntPtr voice,
        ref RvVoiceConnectInfo info);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int rv_voice_tick(IntPtr voice, uint nowMs);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern uint rv_voice_get_required_frame_samples(IntPtr voice);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr rv_voice_result_to_string(int result);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int rv_voice_submit_capture_pcm(
        IntPtr voice,
        short[] samples,
        uint sampleCount);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int rv_voice_ingest_packet(
        IntPtr voice,
        byte[] data,
        uint size,
        uint nowMs);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int rv_voice_poll_outgoing(
        IntPtr voice,
        byte[] outBuffer,
        uint outCapacity,
        out uint outSize);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int rv_voice_poll_event_flat(
        IntPtr voice,
        out RvVoiceEventFlat outEvent,
        short[] outPcm,
        uint outPcmCapacity,
        byte[] outMessage,
        uint outMessageCapacity);
}

internal enum RvVoiceCaptureMode : int
{
    PushToTalkOnly = 0,
    AlwaysOn = 1
}

internal enum RvVoiceEventType : uint
{
    None = 0,
    Log = 1,
    Connected = 2,
    Disconnected = 3,
    Speaking = 4,
    PcmFrame = 5,
    Error = 6
}

[StructLayout(LayoutKind.Sequential)]
internal struct RvVoiceConfig
{
    public uint api_version;
    public uint sample_rate_hz;
    public uint frame_ms;
    public uint max_players;
    public uint jitter_target_ms;
    public uint jitter_max_ms;
    public RvVoiceCaptureMode capture_mode;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
    public uint[] reserved_u32;
}

[StructLayout(LayoutKind.Sequential)]
internal struct RvVoiceConnectInfo
{
    public ulong session_id;
    public ushort player_id;
    public IntPtr relay_host;
    public ushort relay_port;
    public ushort local_port;
    public IntPtr token;
}

[StructLayout(LayoutKind.Sequential)]
internal struct RvVoiceEventFlat
{
    public uint type;

    public int level;
    public int code;

    public ushort speaker_id;
    public byte is_speaking;
    public byte channels;

    public byte flags;
    public byte radio_channel;

    private byte reserved0;
    private byte reserved1;

    public uint sample_rate;
    public uint sample_count;

    public uint message_size;
}