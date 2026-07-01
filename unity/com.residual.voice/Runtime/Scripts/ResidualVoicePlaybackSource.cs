using System;
using System.Collections.Generic;
using UnityEngine;

namespace Residual.Voice
{
    [DisallowMultipleComponent]
    [RequireComponent(typeof(AudioSource))]
    public sealed class ResidualVoicePlaybackSource : MonoBehaviour
    {
        private readonly object _sync = new object();
        private readonly Dictionary<ushort, ResidualVoicePlaybackBuffer> _speakerBuffers =
            new Dictionary<ushort, ResidualVoicePlaybackBuffer>();

        private ResidualVoiceClient _client;
        private AudioSource _audioSource;
        private AudioClip _streamingClip;

        [Header("Playback")]
        [SerializeField]
        private int playbackSampleRateHz = 48000;

            
        [SerializeField]
        private int playbackChannels = 1;

        [SerializeField]
        private int bufferCapacitySamples = 48000;

        [SerializeField]
        [Range(0.0f, 2.0f)]
        private float gain = 1.0f;

        [SerializeField]
        private float lastQueuedPeakDebug;

        [SerializeField]
        private float lastMixedPeakDebug;

        [SerializeField]
        private int silentQueuedFrameCountDebug;

        [SerializeField]
        private int nonSilentQueuedFrameCountDebug;

        [SerializeField]
        private bool autoPlayAudioSource = true;

        [SerializeField]
        private bool keepAudioSourceAlive = true;

        [SerializeField]
        private bool logPlaybackState;

        [Header("Runtime Debug")]
        [SerializeField]
        private int speakerCountDebug;

        [SerializeField]
        private int totalAvailableSamplesDebug;

        [SerializeField]
        private int queuedFrameCountDebug;

        [SerializeField]
        private int queuedSampleCountDebug;

        [SerializeField]
        private int audioReadCallbackCountDebug;

        [SerializeField]
        private int mixedSampleCountDebug;

        [SerializeField]
        private int lastAudioReadSampleCountDebug;

        [SerializeField]
        private bool audioSourceIsPlayingDebug;

        [SerializeField]
        private string playbackStatusDebug = "Not started";

        public int BufferCapacitySamples
        {
            get => bufferCapacitySamples;
            set
            {
                if (value <= 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(value));
                }

                bufferCapacitySamples = value;
            }
        }

        public float Gain
        {
            get => gain;
            set => gain = Mathf.Max(0.0f, value);
        }

        public int SpeakerCount
        {
            get
            {
                lock (_sync)
                {
                    return _speakerBuffers.Count;
                }
            }
        }

        public int TotalAvailableSamples
        {
            get
            {
                lock (_sync)
                {
                    return GetTotalAvailableSamplesNoLock();
                }
            }
        }

        private void Awake()
        {
            EnsureAudioSource();
        }

        private void OnEnable()
        {
            EnsureAudioSource();

            if (autoPlayAudioSource)
            {
                EnsurePlaying();
            }
        }

        private void Update()
        {
            if (keepAudioSourceAlive)
            {
                EnsurePlaying();
            }

            speakerCountDebug = SpeakerCount;
            totalAvailableSamplesDebug = TotalAvailableSamples;
            audioSourceIsPlayingDebug = _audioSource != null && _audioSource.isPlaying;
        }

        private void OnDestroy()
        {
            DetachClient();
            Clear();
        }

        public void AttachClient(ResidualVoiceClient client)
        {
            if (_client == client)
            {
                return;
            }

            DetachClient();

            _client = client ?? throw new ArgumentNullException(nameof(client));
            _client.PcmFrameReady += OnPcmFrameReady;

            playbackStatusDebug = "Client attached";
        }

        public void DetachClient()
        {
            if (_client == null)
            {
                return;
            }

            _client.PcmFrameReady -= OnPcmFrameReady;
            _client = null;

            playbackStatusDebug = "Client detached";
        }

        public void Clear()
        {
            lock (_sync)
            {
                foreach (var buffer in _speakerBuffers.Values)
                {
                    buffer.Clear();
                }

                _speakerBuffers.Clear();
            }

            playbackStatusDebug = "Cleared";
        }

        public void ClearSpeaker(ushort speakerId)
        {
            lock (_sync)
            {
                if (_speakerBuffers.TryGetValue(speakerId, out var buffer))
                {
                    buffer.Clear();
                    _speakerBuffers.Remove(speakerId);
                }
            }
        }

        public int GetAvailableSamples(ushort speakerId)
        {
            lock (_sync)
            {
                return _speakerBuffers.TryGetValue(speakerId, out var buffer)
                    ? buffer.AvailableSamples
                    : 0;
            }
        }

        public void QueueFrame(ResidualVoicePcmFrame frame)
        {
            if (frame.Samples == null || frame.Samples.Length == 0)
            {
                return;
            }

            ResidualVoicePlaybackBuffer buffer;

            lock (_sync)
            {
                if (!_speakerBuffers.TryGetValue(frame.SpeakerId, out buffer))
                {
                    buffer = new ResidualVoicePlaybackBuffer(bufferCapacitySamples);
                    _speakerBuffers.Add(frame.SpeakerId, buffer);
                }
            }

            var peak = GetPcm16Peak(frame.Samples);
            lastQueuedPeakDebug = peak;

            if (peak <= 0.001f)
                {
                    silentQueuedFrameCountDebug++;
                }
            else
                {
                    nonSilentQueuedFrameCountDebug++;
                }

            var written = buffer.Enqueue(frame.Samples, 0, frame.Samples.Length);

            queuedFrameCountDebug++;
            queuedSampleCountDebug += written;
            playbackStatusDebug = $"Queued speaker {frame.SpeakerId}, samples={written}, peak={peak:0.000000}";
        }

        private void OnPcmFrameReady(ResidualVoicePcmFrame frame)
        {
            QueueFrame(frame);
        }

        private void OnAudioRead(float[] data)
        {
            if (data == null || data.Length == 0)
            {
                return;
            }

            Array.Clear(data, 0, data.Length);

            var channels = Mathf.Max(1, playbackChannels);
            var frameCount = data.Length / channels;

            if (frameCount <= 0)
            {
                return;
            }

            var mixedFrames = 0;

            lock (_sync)
            {
                foreach (var buffer in _speakerBuffers.Values)
                {
                    var read = buffer.MixIntoInterleavedFloat(
                        data,
                        destinationOffset: 0,
                        destinationChannels: channels,
                        frameCount: frameCount,
                        gain: gain);

                    if (read > mixedFrames)
                    {
                        mixedFrames = read;
                    }
                }
            }

            lastMixedPeakDebug = GetFloatPeak(data);

            audioReadCallbackCountDebug++;
            lastAudioReadSampleCountDebug = data.Length;
            mixedSampleCountDebug += mixedFrames;
        }

        private void OnAudioSetPosition(int newPosition)
        {
            // Streaming clip position callback intentionally unused.
        }

        private void EnsureAudioSource()
        {
            if (_audioSource == null)
            {
                _audioSource = GetComponent<AudioSource>();
            }

            if (playbackChannels <= 0)
            {
                playbackChannels = 1;
            }

            if (playbackSampleRateHz <= 0)
            {
                playbackSampleRateHz = 48000;
            }

            _audioSource.playOnAwake = true;
            _audioSource.loop = true;
            _audioSource.spatialBlend = 0.0f;
            _audioSource.volume = 1.0f;
            _audioSource.mute = false;

            if (_streamingClip == null)
            {
                _streamingClip = AudioClip.Create(
                    "Residual Voice Streaming Playback",
                    playbackSampleRateHz,
                    playbackChannels,
                    playbackSampleRateHz,
                    stream: true,
                    pcmreadercallback: OnAudioRead,
                    pcmsetpositioncallback: OnAudioSetPosition);

                playbackStatusDebug = "Streaming clip created";
            }

            if (_audioSource.clip != _streamingClip)
            {
                _audioSource.clip = _streamingClip;
            }
        }

        private void EnsurePlaying()
        {
            EnsureAudioSource();

            if (!_audioSource.enabled || !gameObject.activeInHierarchy)
            {
                return;
            }

            if (!_audioSource.isPlaying)
            {
                if (logPlaybackState)
                {
                    Debug.Log("ResidualVoicePlaybackSource starting streaming AudioSource.", this);
                }

                _audioSource.Play();
                playbackStatusDebug = "AudioSource playing";
            }
        }
        private static float GetPcm16Peak(short[] samples)
            {
            if (samples == null || samples.Length == 0)
            {
                return 0.0f;
            }

            var peak = 0.0f;

            for (var i = 0; i < samples.Length; i++)
            {
                var value = Mathf.Abs(samples[i] / 32768.0f);

                if (value > peak)
                {
                    peak = value;
                }
            }

            return peak;
            
            }

        private static float GetFloatPeak(float[] samples)
        {
    
                if (samples == null || samples.Length == 0)
                {
                    return 0.0f;
                }

                var peak = 0.0f;

                for (var i = 0; i < samples.Length; i++)
                {

                    var value = Mathf.Abs(samples[i]);

                    if (value > peak)
                    {
                        peak = value;
                    }
                }
            return peak;
        
            }        
        private int GetTotalAvailableSamplesNoLock()
        {
            var total = 0;

            foreach (var buffer in _speakerBuffers.Values)
            {
                total += buffer.AvailableSamples;
            }

            return total;
        }
    }
}