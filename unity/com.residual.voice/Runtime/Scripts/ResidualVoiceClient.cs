using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Residual.Voice
{
    public sealed class ResidualVoiceClient : IDisposable
    {
        private const int DefaultPacketBufferSize = 1500;
        private const int DefaultMessageBufferSize = 1024;

        private readonly byte[] _packetBuffer = new byte[DefaultPacketBufferSize];
        private readonly byte[] _messageBuffer = new byte[DefaultMessageBufferSize];

        private short[] _pcmBuffer = Array.Empty<short>();
        private IntPtr _handle;

        public event Action<byte[], int> PacketReady;
        public event Action<ResidualVoicePcmFrame> PcmFrameReady;
        public event Action<string> LogReceived;
        public event Action<string> ErrorReceived;

        public bool IsCreated => _handle != IntPtr.Zero;

        public uint RequiredFrameSamples
        {
            get
            {
                EnsureCreated();
                return ResidualVoiceNative.rv_voice_get_required_frame_samples(_handle);
            }
        }

        public static uint NativeApiVersion => ResidualVoiceNative.rv_voice_get_api_version();

        public void Create(
            ushort localPlayerId,
            uint sampleRateHz = 48000,
            uint frameMs = 20,
            ushort maxPlayers = 64,
            uint jitterTargetMs = 60,
            uint jitterMaxMs = 200,
            bool alwaysOn = false)
        {
            if (_handle != IntPtr.Zero)
            {
                throw new InvalidOperationException("ResidualVoiceClient is already created.");
            }

            var config = new RvVoiceConfig
            {
                api_version = ResidualVoiceNative.rv_voice_get_api_version(),
                sample_rate_hz = sampleRateHz,
                frame_ms = frameMs,
                max_players = maxPlayers,
                jitter_target_ms = jitterTargetMs,
                jitter_max_ms = jitterMaxMs,
                capture_mode = alwaysOn
                    ? RvVoiceCaptureMode.AlwaysOn
                    : RvVoiceCaptureMode.PushToTalkOnly,
                reserved_u32 = new uint[8]
            };

            _handle = ResidualVoiceNative.rv_voice_create(ref config, IntPtr.Zero, IntPtr.Zero);

            if (_handle == IntPtr.Zero)
            {
                throw new InvalidOperationException("Failed to create Residual voice client.");
            }

            var frameSamples = ResidualVoiceNative.rv_voice_get_required_frame_samples(_handle);
            _pcmBuffer = new short[Math.Max(1, (int)frameSamples)];
        }

        public void Connect(ulong sessionId, ushort localPlayerId)
        {
            EnsureCreated();

            var info = new RvVoiceConnectInfo
            {
                session_id = sessionId,
                player_id = localPlayerId,
                relay_host = IntPtr.Zero,
                relay_port = 0,
                local_port = 0,
                token = IntPtr.Zero
            };

            ThrowIfError(ResidualVoiceNative.rv_voice_connect(_handle, ref info));
        }

        public void Disconnect()
        {
            if (_handle == IntPtr.Zero)
            {
                return;
            }

            ResidualVoiceNative.rv_voice_disconnect(_handle);
        }

        public void Tick(uint nowMs)
        {
            EnsureCreated();

            ThrowIfError(ResidualVoiceNative.rv_voice_tick(_handle, nowMs));

            DrainOutgoingPackets();
            DrainEvents();
        }

        public void SubmitCapturedPcm(short[] samples, int sampleCount)
        {
            EnsureCreated();

            if (samples == null)
            {
                throw new ArgumentNullException(nameof(samples));
            }

            if (sampleCount < 0 || sampleCount > samples.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(sampleCount));
            }

            ThrowIfError(ResidualVoiceNative.rv_voice_submit_capture_pcm(
                _handle,
                samples,
                (uint)sampleCount));
        }

        public void SubmitCapturedPcmAsync(short[] samples, int sampleCount)
        {
            EnsureCreated();

            if (samples == null)
            {
                throw new ArgumentNullException(nameof(samples));
            }

            if (sampleCount < 0 || sampleCount > samples.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(sampleCount));
            }

            ThrowIfError(ResidualVoiceNative.rv_voice_submit_capture_pcm_async(
                _handle,
                samples,
                (uint)sampleCount));
        }

        public void IngestPacket(byte[] packet, int size, uint nowMs)
        {
            EnsureCreated();

            if (packet == null)
            {
                throw new ArgumentNullException(nameof(packet));
            }

            if (size < 0 || size > packet.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(size));
            }

            ThrowIfError(ResidualVoiceNative.rv_voice_ingest_packet(
                _handle,
                packet,
                (uint)size,
                nowMs));
        }

        public void Dispose()
        {
            if (_handle == IntPtr.Zero)
            {
                return;
            }

            ResidualVoiceNative.rv_voice_destroy(_handle);
            _handle = IntPtr.Zero;
        }

        private void DrainOutgoingPackets()
        {
            while (true)
            {
                var result = ResidualVoiceNative.rv_voice_poll_outgoing(
                    _handle,
                    _packetBuffer,
                    (uint)_packetBuffer.Length,
                    out var outSize);

                if (result == 0 || outSize == 0)
                {
                    return;
                }

                ThrowIfError(result);

                var packetCopy = new byte[outSize];
                Buffer.BlockCopy(_packetBuffer, 0, packetCopy, 0, (int)outSize);
                PacketReady?.Invoke(packetCopy, (int)outSize);
            }
        }

        private void DrainEvents()
        {
            while (true)
            {
                Array.Clear(_messageBuffer, 0, _messageBuffer.Length);

                var result = ResidualVoiceNative.rv_voice_poll_event_flat(
                    _handle,
                    out var ev,
                    _pcmBuffer,
                    (uint)_pcmBuffer.Length,
                    _messageBuffer,
                    (uint)_messageBuffer.Length);

                if (result == 0)
                {
                    return;
                }

                ThrowIfError(result);

                var eventType = (ResidualVoiceEventType)ev.type;

                switch (eventType)
                {
                    case ResidualVoiceEventType.Log:
                        LogReceived?.Invoke(ReadNativeMessage(ev.message_size));
                        break;

                    case ResidualVoiceEventType.Error:
                        ErrorReceived?.Invoke(ReadNativeMessage(ev.message_size));
                        break;

                    case ResidualVoiceEventType.PcmFrame:
                        var sampleCount = checked((int)ev.sample_count);

                        if (_pcmBuffer.Length < sampleCount)
                        {
                            _pcmBuffer = new short[sampleCount];
                            break;
                        }

                        var copy = new short[sampleCount];
                        Array.Copy(_pcmBuffer, copy, sampleCount);

                        PcmFrameReady?.Invoke(new ResidualVoicePcmFrame(
                            ev.speaker_id,
                            copy,
                            ev.sample_rate,
                            ev.channels,
                            ev.flags,
                            ev.radio_channel));
                        break;
                }
            }
        }

        private string ReadNativeMessage(uint messageSize)
        {
            var count = Math.Min((int)messageSize, _messageBuffer.Length);
            return Encoding.UTF8.GetString(_messageBuffer, 0, count).TrimEnd('\0');
        }

        private void EnsureCreated()
        {
            if (_handle == IntPtr.Zero)
            {
                throw new InvalidOperationException("ResidualVoiceClient has not been created.");
            }
        }

        private static void ThrowIfError(int result)
        {
            if (result >= 0)
            {
                return;
            }

            var ptr = ResidualVoiceNative.rv_voice_result_to_string(result);
            var message = ptr == IntPtr.Zero
                ? $"Residual voice native call failed with code {result}."
                : Marshal.PtrToStringAnsi(ptr) ?? $"Residual voice native call failed with code {result}.";

            throw new InvalidOperationException(message);
        }
    }
}