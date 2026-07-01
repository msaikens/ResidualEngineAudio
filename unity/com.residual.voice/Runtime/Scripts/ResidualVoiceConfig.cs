using System;
using System.Runtime.InteropServices;

namespace Residual.Voice
{
    public enum ResidualVoiceResult
    {
        Ok = 0,
        InvalidArgument = -1,
        OutOfMemory = -2,
        NotInitialized = -3,
        NotConnected = -4,
        Internal = -5
    }

    internal enum RvVoiceCaptureMode : int
    {
        PushToTalkOnly = 0,
        AlwaysOn = 1
    }

    public enum ResidualVoiceEventType : uint
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

    public readonly struct ResidualVoicePcmFrame
    {
        public ResidualVoicePcmFrame(
            ushort speakerId,
            short[] samples,
            uint sampleRate,
            byte channels,
            byte flags,
            byte radioChannel)
        {
            SpeakerId = speakerId;
            Samples = samples;
            SampleRate = sampleRate;
            Channels = channels;
            Flags = flags;
            RadioChannel = radioChannel;
        }

        public ushort SpeakerId { get; }
        public short[] Samples { get; }
        public uint SampleRate { get; }
        public byte Channels { get; }
        public byte Flags { get; }
        public byte RadioChannel { get; }
    }
}