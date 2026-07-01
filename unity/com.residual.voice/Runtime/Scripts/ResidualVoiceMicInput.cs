using System;
using UnityEngine;

namespace Residual.Voice
{
    [DisallowMultipleComponent]
    public sealed class ResidualVoiceMicInput : MonoBehaviour
    {
        private ResidualVoiceClient _client;
        private AudioClip _microphoneClip;

        private float[] _floatReadBuffer = Array.Empty<float>();
        private short[] _monoScratchBuffer = Array.Empty<short>();
        private short[] _frameSubmitBuffer = Array.Empty<short>();

        private int _frameWriteCount;
        private int _requiredFrameSamples;
        private int _lastReadPosition;
        private bool _isRecording;

        [Header("Microphone")]
        [SerializeField]
        private string deviceName = string.Empty;

        [SerializeField]
        private int sampleRateHz = 48000;

        [SerializeField]
        private int clipLengthSeconds = 2;

        [SerializeField]
        private bool autoStart = true;

        [SerializeField]
        private bool submitAsync = true;

        [SerializeField]
        private int maxSamplesPerUpdate = 48000;

        [SerializeField]
        private bool logMicrophoneInfo;

        [Header("Runtime Debug")]
        [SerializeField]
        private string resolvedDeviceDebug = string.Empty;

        [SerializeField]
        private int microphonePositionDebug;

        [SerializeField]
        private int lastReadPositionDebug;

        [SerializeField]
        private int requiredFrameSamplesDebug;

        [SerializeField]
        private int pendingFrameSamplesDebug;

        [SerializeField]
        private int submittedFrameCountDebug;

        [SerializeField]
        private int submittedSampleCountDebug;

        [SerializeField]
        private float peakLevelDebug;

        [SerializeField]
        private string lastStatusDebug = "Not started";

        public bool IsRecording => _isRecording;

        public string DeviceName
        {
            get => deviceName;
            set => deviceName = value ?? string.Empty;
        }

        public int SampleRateHz
        {
            get => sampleRateHz;
            set
            {
                if (value <= 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(value));
                }

                sampleRateHz = value;
            }
        }

        public int ClipLengthSeconds
        {
            get => clipLengthSeconds;
            set
            {
                if (value <= 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(value));
                }

                clipLengthSeconds = value;
            }
        }

        public int MaxSamplesPerUpdate
        {
            get => maxSamplesPerUpdate;
            set
            {
                if (value <= 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(value));
                }

                maxSamplesPerUpdate = value;
            }
        }

        private void Start()
        {
            if (autoStart)
            {
                StartCapture();
            }
        }

        private void Update()
        {
            if (!_isRecording || _microphoneClip == null)
            {
                return;
            }

            PumpMicrophone();
        }

        private void OnDestroy()
        {
            StopCapture();
            DetachClient();
        }

        public void AttachClient(ResidualVoiceClient client)
        {
            _client = client ?? throw new ArgumentNullException(nameof(client));
            ConfigureFrameBuffer();
            lastStatusDebug = "Client attached";
        }

        public void DetachClient()
        {
            _client = null;
            _requiredFrameSamples = 0;
            _frameWriteCount = 0;
            requiredFrameSamplesDebug = 0;
            pendingFrameSamplesDebug = 0;
            lastStatusDebug = "Client detached";
        }

        public void StartCapture()
        {
            if (_isRecording)
            {
                return;
            }

            if (_client != null)
            {
                ConfigureFrameBuffer();
            }

            var resolvedDevice = ResolveDeviceName();
            resolvedDeviceDebug = resolvedDevice ?? "Default microphone";

            if (Microphone.devices == null || Microphone.devices.Length == 0)
            {
                lastStatusDebug = "No Unity microphone devices found";
                Debug.LogWarning("ResidualVoiceMicInput found no microphone devices.", this);
                return;
            }

            if (logMicrophoneInfo)
            {
                Debug.Log($"ResidualVoiceMicInput devices: {string.Join(", ", Microphone.devices)}", this);
                Debug.Log($"ResidualVoiceMicInput starting microphone: '{resolvedDeviceDebug}'", this);
            }

            _microphoneClip = Microphone.Start(
                resolvedDevice,
                loop: true,
                lengthSec: clipLengthSeconds,
                frequency: sampleRateHz);

            if (_microphoneClip == null)
            {
                lastStatusDebug = "Microphone.Start returned null";
                throw new InvalidOperationException("Unity Microphone.Start returned null.");
            }

            _lastReadPosition = 0;
            _frameWriteCount = 0;
            microphonePositionDebug = 0;
            lastReadPositionDebug = 0;
            pendingFrameSamplesDebug = 0;
            submittedFrameCountDebug = 0;
            submittedSampleCountDebug = 0;
            peakLevelDebug = 0.0f;
            _isRecording = true;
            lastStatusDebug = "Recording started";
        }

        public void StopCapture()
        {
            if (!_isRecording)
            {
                return;
            }

            var resolvedDevice = ResolveDeviceName();

            if (Microphone.IsRecording(resolvedDevice))
            {
                Microphone.End(resolvedDevice);
            }

            _microphoneClip = null;
            _lastReadPosition = 0;
            _frameWriteCount = 0;
            microphonePositionDebug = 0;
            lastReadPositionDebug = 0;
            pendingFrameSamplesDebug = 0;
            _isRecording = false;
            lastStatusDebug = "Recording stopped";
        }

        public void RestartCapture()
        {
            StopCapture();
            StartCapture();
        }

        public static string[] GetAvailableDevices()
        {
            return Microphone.devices;
        }

        private string ResolveDeviceName()
        {
            return string.IsNullOrWhiteSpace(deviceName)
                ? null
                : deviceName;
        }

        private void ConfigureFrameBuffer()
        {
            if (_client == null || !_client.IsCreated)
            {
                lastStatusDebug = "Waiting for voice client";
                return;
            }

            var required = checked((int)_client.RequiredFrameSamples);

            if (required <= 0)
            {
                throw new InvalidOperationException("Residual voice required frame samples returned zero.");
            }

            _requiredFrameSamples = required;
            requiredFrameSamplesDebug = required;

            if (_frameSubmitBuffer.Length != _requiredFrameSamples)
            {
                _frameSubmitBuffer = new short[_requiredFrameSamples];
                _frameWriteCount = 0;
            }
        }

        private void PumpMicrophone()
        {
            if (_client == null || !_client.IsCreated)
            {
                lastStatusDebug = "Recording, but no ResidualVoiceClient attached";
                return;
            }

            if (_requiredFrameSamples <= 0)
            {
                ConfigureFrameBuffer();

                if (_requiredFrameSamples <= 0)
                {
                    return;
                }
            }

            var resolvedDevice = ResolveDeviceName();
            var currentPosition = Microphone.GetPosition(resolvedDevice);

            microphonePositionDebug = currentPosition;
            lastReadPositionDebug = _lastReadPosition;

            if (currentPosition < 0)
            {
                lastStatusDebug = "Microphone.GetPosition returned negative";
                return;
            }

            if (currentPosition == _lastReadPosition)
            {
                lastStatusDebug = "Waiting for microphone samples";
                return;
            }

            var clipSamples = _microphoneClip.samples;
            var channels = _microphoneClip.channels;

            if (clipSamples <= 0 || channels <= 0)
            {
                lastStatusDebug = "Microphone clip has invalid sample/channel count";
                return;
            }

            if (currentPosition > _lastReadPosition)
            {
                ReadAndAccumulate(_lastReadPosition, currentPosition - _lastReadPosition, channels);
            }
            else
            {
                ReadAndAccumulate(_lastReadPosition, clipSamples - _lastReadPosition, channels);

                if (currentPosition > 0)
                {
                    ReadAndAccumulate(0, currentPosition, channels);
                }
            }

            _lastReadPosition = currentPosition;
            lastReadPositionDebug = _lastReadPosition;
            pendingFrameSamplesDebug = _frameWriteCount;
        }

        private void ReadAndAccumulate(int startSample, int sampleCount, int channels)
        {
            if (sampleCount <= 0)
            {
                return;
            }

            var remaining = sampleCount;
            var readStart = startSample;

            while (remaining > 0)
            {
                var chunkSamples = Mathf.Min(remaining, maxSamplesPerUpdate);
                var floatSampleCount = chunkSamples * channels;

                EnsureFloatBuffer(floatSampleCount);
                EnsureMonoScratchBuffer(chunkSamples);

                if (!_microphoneClip.GetData(_floatReadBuffer, readStart))
                {
                    lastStatusDebug = "MicrophoneClip.GetData failed";
                    return;
                }

                UpdatePeakLevel(_floatReadBuffer, floatSampleCount);

                ResidualVoicePcmUtility.DownmixInterleavedFloatToMonoPcm16(
                    _floatReadBuffer,
                    sourceOffset: 0,
                    sourceChannels: channels,
                    destination: _monoScratchBuffer,
                    destinationOffset: 0,
                    frameCount: chunkSamples);

                AccumulateAndSubmitFrames(_monoScratchBuffer, chunkSamples);

                remaining -= chunkSamples;
                readStart += chunkSamples;
            }
        }

        private void AccumulateAndSubmitFrames(short[] samples, int sampleCount)
        {
            var offset = 0;

            while (sampleCount > 0)
            {
                var needed = _requiredFrameSamples - _frameWriteCount;
                var copyCount = Mathf.Min(needed, sampleCount);

                Array.Copy(
                    samples,
                    offset,
                    _frameSubmitBuffer,
                    _frameWriteCount,
                    copyCount);

                _frameWriteCount += copyCount;
                pendingFrameSamplesDebug = _frameWriteCount;

                offset += copyCount;
                sampleCount -= copyCount;

                if (_frameWriteCount == _requiredFrameSamples)
                {
                    SubmitFrame();
                    _frameWriteCount = 0;
                    pendingFrameSamplesDebug = 0;
                }
            }
        }

        private void SubmitFrame()
        {
            if (_client == null || !_client.IsCreated)
            {
                lastStatusDebug = "Skipped submit: no client";
                return;
            }

            if (submitAsync)
            {
                _client.SubmitCapturedPcmAsync(_frameSubmitBuffer, _requiredFrameSamples);
            }
            else
            {
                _client.SubmitCapturedPcm(_frameSubmitBuffer, _requiredFrameSamples);
            }

            submittedFrameCountDebug++;
            submittedSampleCountDebug += _requiredFrameSamples;
            lastStatusDebug = "Submitted PCM frame";
        }

        private void UpdatePeakLevel(float[] samples, int sampleCount)
        {
            var peak = 0.0f;

            for (var i = 0; i < sampleCount; i++)
            {
                var abs = Mathf.Abs(samples[i]);

                if (abs > peak)
                {
                    peak = abs;
                }
            }

            peakLevelDebug = peak;
        }

        private void EnsureFloatBuffer(int sampleCount)
        {
            if (_floatReadBuffer.Length < sampleCount)
            {
                _floatReadBuffer = new float[sampleCount];
            }
        }

        private void EnsureMonoScratchBuffer(int sampleCount)
        {
            if (_monoScratchBuffer.Length < sampleCount)
            {
                _monoScratchBuffer = new short[sampleCount];
            }
        }
    }
}