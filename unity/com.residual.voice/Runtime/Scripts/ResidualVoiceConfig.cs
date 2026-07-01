using System;
using System.Runtime.InteropServices;

namespace Residual.Voice
{

// This enum represents the different result codes that can be returned by the Residual Voice system.
// Each result code corresponds to a specific outcome of an operation, such as success, invalid argument, 
// out of memory, not initialized, not connected, or internal error.

    public enum ResidualVoiceResult
    {
        Ok = 0,
        InvalidArgument = -1,
        OutOfMemory = -2,
        NotInitialized = -3,
        NotConnected = -4,
        Internal = -5
    }

// This enum represents the different capture modes that can be used in the Residual Voice system. 
// The capture mode determines how the voice data is captured and transmitted, either through push-to-talk or always-on mode.

    internal enum RvVoiceCaptureMode : int
    {
        PushToTalkOnly = 0,
        AlwaysOn = 1
    }

// This enum represents the different types of events that can occur in the Residual Voice system.
// Each event type corresponds to a specific action or state change, such as a player connecting or disconnecting,
// a player starting or stopping speaking, or an error occurring. The enum values are defined as unsigned integers (uint).

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

// This struct is used to represent the data received from the native library for a voice event.
// It is marked with StructLayout to ensure that the fields are laid out in memory in the same order as they are defined, 
// which is important for interop with native code.

// The fields in this struct correspond to the data that is sent from the native library when a voice event occurs,
// such as when a player starts or stops speaking, or when a PCM frame is received.

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

// This struct is used to represent the configuration settings for the Residual Voice system.
// It is marked with StructLayout to ensure that the fields are laid out in memory in the
// same order as they are defined, which is important for interop with native code.    

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
    internal struct RvVec3
    {
        public float x;
        public float y;
        public float z;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct RvVoicePlayerState
    {
        public RvVec3 position;
        public RvVec3 forward;

        public byte ptt_down;
        public byte radio_enabled;
        public byte radio_channel;
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