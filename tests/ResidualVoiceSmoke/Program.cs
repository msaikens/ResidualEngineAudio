using System;
using System.Runtime.InteropServices;

internal static class Program
{
    private static void Main()
    {
        var config = new RvVoiceConfig
        {
            api_version = Native.rv_voice_get_api_version(),
            sample_rate_hz = 48000,
            frame_ms = 20,
            max_players = 16,
            jitter_target_ms = 60,
            jitter_max_ms = 200,
            capture_mode = RvVoiceCaptureMode.PushToTalkOnly,
            reserved_u32 = new uint[8]
        };

        Console.WriteLine($"Native API version: {config.api_version}");

        var handle = Native.rv_voice_create(ref config, IntPtr.Zero, IntPtr.Zero);

        if (handle == IntPtr.Zero)
        {
            throw new Exception("rv_voice_create returned null.");
        }

        Console.WriteLine($"Frame samples: {Native.rv_voice_get_required_frame_samples(handle)}");

        Native.rv_voice_destroy(handle);

        Console.WriteLine("Smoke test passed.");
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
    internal static extern uint rv_voice_get_required_frame_samples(IntPtr voice);
}

internal enum RvVoiceCaptureMode : int
{
    PushToTalkOnly = 0,
    AlwaysOn = 1
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