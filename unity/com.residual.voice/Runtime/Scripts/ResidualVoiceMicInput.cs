using System;
using UnityEngine;

namespace Residual.Voice
{
    [DisallowMultipleComponent]
    public sealed class ResidualVoiceMicInput : MonoBehaviour
    {
        private ResidualVoiceClient _client;
        private AudioClip _microphoneClip;
        private float[] _floatReadBuffer = [];
        private short[] _pcmSubmitBuffer = [];

        private int _lastReadPosition;
        private bool _isRecording;

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
            if (!_isRecording || _microphoneClip == null || _client == null)
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
        }

        public void DetachClient()
        {
            _client = null;
        }

        public void StartCapture()
        {
            if (_isRecording)
            {
                return;
            }

            var resolvedDevice = ResolveDeviceName();

            if (logMicrophoneInfo)
            {
                Debug.Log($"ResidualVoiceMicInput starting microphone: '{resolvedDevice ?? "default"}'");
            }

            _microphoneClip = Microphone.Start(
                resolvedDevice,
                loop: true,
                lengthSec: clipLengthSeconds,
                frequency: sampleRateHz);

            if (_microphoneClip == null)
            {
                throw new InvalidOperationException("Unity Microphone.Start returned null.");
            }

            _lastReadPosition = 0;
            _isRecording = true;
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
            _isRecording = false;
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

        private void PumpMicrophone()
        {
            var resolvedDevice = ResolveDeviceName();
            var currentPosition = Microphone.GetPosition(resolvedDevice);

            if (currentPosition < 0)
            {
                return;
            }

            if (currentPosition == _lastReadPosition)
            {
                return;
            }

            var clipSamples = _microphoneClip.samples;
            var channels = _microphoneClip.channels;

            if (clipSamples <= 0 || channels <= 0)
            {
                return;
            }

            if (currentPosition > _lastReadPosition)
            {
                ReadAndSubmit(_lastReadPosition, currentPosition - _lastReadPosition, channels);
            }
            else
            {
                ReadAndSubmit(_lastReadPosition, clipSamples - _lastReadPosition, channels);

                if (currentPosition > 0)
                {
                    ReadAndSubmit(0, currentPosition, channels);
                }
            }

            _lastReadPosition = currentPosition;
        }

        private void ReadAndSubmit(int startSample, int sampleCount, int channels)
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
                EnsurePcmBuffer(chunkSamples);

                if (!_microphoneClip.GetData(_floatReadBuffer, readStart))
                {
                    return;
                }

                ResidualVoicePcmUtility.DownmixInterleavedFloatToMonoPcm16(
                    _floatReadBuffer,
                    sourceOffset: 0,
                    sourceChannels: channels,
                    destination: _pcmSubmitBuffer,
                    destinationOffset: 0,
                    frameCount: chunkSamples);

                if (submitAsync)
                {
                    _client.SubmitCapturedPcmAsync(_pcmSubmitBuffer, chunkSamples);
                }
                else
                {
                    _client.SubmitCapturedPcm(_pcmSubmitBuffer, chunkSamples);
                }

                remaining -= chunkSamples;
                readStart += chunkSamples;
            }
        }

        private void EnsureFloatBuffer(int sampleCount)
        {
            if (_floatReadBuffer.Length < sampleCount)
            {
                _floatReadBuffer = new float[sampleCount];
            }
        }

        private void EnsurePcmBuffer(int sampleCount)
        {
            if (_pcmSubmitBuffer.Length < sampleCount)
            {
                _pcmSubmitBuffer = new short[sampleCount];
            }
        }
    }
}