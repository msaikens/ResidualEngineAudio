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

        [SerializeField]
        private int bufferCapacitySamples = 48000;

        [SerializeField]
        private bool clearOutputBeforeMix = true;

        [SerializeField]
        [Range(0.0f, 2.0f)]
        private float gain = 1.0f;

        [SerializeField]
        private bool autoPlayAudioSource = true;

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

        public bool ClearOutputBeforeMix
        {
            get => clearOutputBeforeMix;
            set => clearOutputBeforeMix = value;
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

        private void Awake()
        {
            var source = GetComponent<AudioSource>();

            source.playOnAwake = true;
            source.loop = true;
            source.spatialBlend = 0.0f;

            if (source.clip == null)
            {
                source.clip = AudioClip.Create(
                    "Residual Voice Playback",
                    48000,
                    1,
                    48000,
                    stream: false);
            }

            if (autoPlayAudioSource && !source.isPlaying)
            {
                source.Play();
            }
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
        }

        public void DetachClient()
        {
            if (_client == null)
            {
                return;
            }

            _client.PcmFrameReady -= OnPcmFrameReady;
            _client = null;
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

            buffer.Enqueue(frame.Samples, 0, frame.Samples.Length);
        }

        private void OnPcmFrameReady(ResidualVoicePcmFrame frame)
        {
            QueueFrame(frame);
        }

        private void OnAudioFilterRead(float[] data, int channels)
        {
            if (data == null || data.Length == 0 || channels <= 0)
            {
                return;
            }

            if (clearOutputBeforeMix)
            {
                Array.Clear(data, 0, data.Length);
            }

            var frameCount = data.Length / channels;

            if (frameCount <= 0)
            {
                return;
            }

            lock (_sync)
            {
                foreach (var buffer in _speakerBuffers.Values)
                {
                    buffer.MixIntoInterleavedFloat(
                        data,
                        destinationOffset: 0,
                        destinationChannels: channels,
                        frameCount: frameCount,
                        gain: gain);
                }
            }
        }
    }
}