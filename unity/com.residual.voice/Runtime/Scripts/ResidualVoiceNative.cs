using System;
using System.Runtime.InteropServices;

namespace Residual.Voice
{
    internal static class ResidualVoiceNative
    {
#if UNITY_IOS && !UNITY_EDITOR
        private const string LibraryName = "__Internal";
#else
        private const string LibraryName = "residual_voice";
#endif

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint rv_voice_get_api_version();

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rv_voice_create(
            ref RvVoiceConfig config,
            IntPtr allocators,
            IntPtr callbacks);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rv_voice_destroy(IntPtr voice);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rv_voice_connect(IntPtr voice, ref RvVoiceConnectInfo info);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rv_voice_disconnect(IntPtr voice);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rv_voice_tick(IntPtr voice, uint nowMs);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint rv_voice_get_required_frame_samples(IntPtr voice);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint rv_voice_get_sample_rate(IntPtr voice);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint rv_voice_get_frame_ms(IntPtr voice);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint rv_voice_get_max_players(IntPtr voice);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rv_voice_result_to_string(int result);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rv_voice_submit_capture_pcm(
            IntPtr voice,
            short[] samples,
            uint sampleCount);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rv_voice_submit_capture_pcm_async(
            IntPtr voice,
            short[] samples,
            uint sampleCount);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rv_voice_ingest_packet(
            IntPtr voice,
            byte[] data,
            uint size,
            uint nowMs);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rv_voice_poll_outgoing(
            IntPtr voice,
            byte[] outBuffer,
            uint outCapacity,
            out uint outSize);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rv_voice_poll_event_flat(
            IntPtr voice,
            out RvVoiceEventFlat outEvent,
            short[] outPcm,
            uint outPcmCapacity,
            byte[] outMessage,
            uint outMessageCapacity);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rv_voice_set_local_state(IntPtr voice, ref RvVoicePlayerState state);
    }
}