using UnityEngine;

namespace Residual.Voice
{
    [DisallowMultipleComponent]
    [RequireComponent(typeof(AudioSource))]
    public sealed class ResidualVoicePlaybackProbe : MonoBehaviour
    {
        private AudioSource _audioSource;
        private AudioClip _clip;
        private double _phase;

        [SerializeField]
        private bool playTone = true;

        [SerializeField]
        private int sampleRateHz = 48000;

        [SerializeField]
        private float frequencyHz = 440.0f;

        [SerializeField]
        [Range(0.0f, 1.0f)]
        private float volume = 0.15f;

        [SerializeField]
        private bool logState;

        [Header("Runtime Debug")]
        [SerializeField]
        private bool audioSourceIsPlayingDebug;

        [SerializeField]
        private int callbackCountDebug;

        [SerializeField]
        private int lastSampleCountDebug;

        private void Awake()
        {
            EnsureAudioSource();
        }

        private void OnEnable()
        {
            EnsureAudioSource();
            EnsurePlaying();
        }

        private void Update()
        {
            EnsurePlaying();

            audioSourceIsPlayingDebug = _audioSource != null && _audioSource.isPlaying;
        }

        private void OnDisable()
        {
            if (_audioSource != null)
            {
                _audioSource.Stop();
            }
        }

        private void EnsureAudioSource()
        {
            if (_audioSource == null)
            {
                _audioSource = GetComponent<AudioSource>();
            }

            _audioSource.playOnAwake = true;
            _audioSource.loop = true;
            _audioSource.spatialBlend = 0.0f;
            _audioSource.volume = 1.0f;
            _audioSource.mute = false;

            if (_clip == null)
            {
                _clip = AudioClip.Create(
                    "Residual Voice Playback Probe",
                    sampleRateHz,
                    1,
                    sampleRateHz,
                    stream: true,
                    pcmreadercallback: OnAudioRead,
                    pcmsetpositioncallback: OnAudioSetPosition);
            }

            if (_audioSource.clip != _clip)
            {
                _audioSource.clip = _clip;
            }
        }

        private void EnsurePlaying()
        {
            if (_audioSource == null)
            {
                return;
            }

            if (!_audioSource.isPlaying)
            {
                if (logState)
                {
                    Debug.Log("ResidualVoicePlaybackProbe starting AudioSource.", this);
                }

                _audioSource.Play();
            }
        }

        private void OnAudioRead(float[] data)
        {
            callbackCountDebug++;
            lastSampleCountDebug = data != null ? data.Length : 0;

            if (data == null)
            {
                return;
            }

            if (!playTone)
            {
                for (var i = 0; i < data.Length; i++)
                {
                    data[i] = 0.0f;
                }

                return;
            }

            var phaseStep = 2.0 * Mathf.PI * frequencyHz / sampleRateHz;

            for (var i = 0; i < data.Length; i++)
            {
                data[i] = Mathf.Sin((float)_phase) * volume;

                _phase += phaseStep;

                if (_phase > 2.0 * Mathf.PI)
                {
                    _phase -= 2.0 * Mathf.PI;
                }
            }
        }

        private void OnAudioSetPosition(int newPosition)
        {
            _phase = 0.0;
        }
    }
}