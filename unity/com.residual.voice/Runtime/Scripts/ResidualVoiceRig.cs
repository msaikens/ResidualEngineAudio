using System;
using UnityEngine;

namespace Residual.Voice
{
    [DisallowMultipleComponent]
    public sealed class ResidualVoiceRig : MonoBehaviour
    {
        private ResidualVoiceClient _client;
        private IResidualVoiceTransport _transport;
        private ResidualVoiceTransportBinding _binding;

        [Header("Session")]
        [SerializeField]
        private ulong sessionId = 12345;

        [SerializeField]
        private ushort localPlayerId = 1;

        [Header("Voice Config")]
        [SerializeField]
        private uint sampleRateHz = 48000;

        [SerializeField]
        private uint frameMs = 20;

        [SerializeField]
        private ushort maxPlayers = 64;

        [SerializeField]
        private uint jitterTargetMs = 60;

        [SerializeField]
        private uint jitterMaxMs = 200;

        [SerializeField]
        private bool alwaysOn;

        [Header("Components")]
        [SerializeField]
        private ResidualVoiceMicInput micInput;

        [SerializeField]
        private ResidualVoicePlaybackSource playbackSource;

        [Header("Lifecycle")]
        [SerializeField]
        private bool createOnStart = true;

        [SerializeField]
        private bool connectOnStart = true;

        [SerializeField]
        private bool startMicOnStart = true;

        [SerializeField]
        private bool useLoopbackTransport = true;

        [SerializeField]
        private bool logNativeMessages = true;

        public ResidualVoiceClient Client => _client;

        public IResidualVoiceTransport Transport => _transport;

        public bool IsRunning => _client != null && _client.IsCreated;

        private void Awake()
        {
            if (micInput == null)
            {
                micInput = GetComponent<ResidualVoiceMicInput>();
            }

            if (playbackSource == null)
            {
                playbackSource = GetComponent<ResidualVoicePlaybackSource>();
            }
        }

        private void Start()
        {
            if (createOnStart)
            {
                Create();
            }

            if (connectOnStart)
            {
                Connect();
            }

            if (startMicOnStart && micInput != null)
            {
                micInput.StartCapture();
            }
        }

        private void Update()
        {
            if (_binding == null)
            {
                return;
            }

            _binding.Tick(GetNowMs());
        }

        private void OnDestroy()
        {
            Shutdown();
        }

        public void Create()
        {
            if (_client != null)
            {
                return;
            }

            _client = new ResidualVoiceClient();

            _client.Create(
                localPlayerId: localPlayerId,
                sampleRateHz: sampleRateHz,
                frameMs: frameMs,
                maxPlayers: maxPlayers,
                jitterTargetMs: jitterTargetMs,
                jitterMaxMs: jitterMaxMs,
                alwaysOn: alwaysOn);

            if (logNativeMessages)
            {
                _client.LogReceived += OnNativeLogReceived;
                _client.ErrorReceived += OnNativeErrorReceived;
            }

            if (micInput != null)
            {
                micInput.AttachClient(_client);
            }

            if (playbackSource != null)
            {
                playbackSource.AttachClient(_client);
            }

            if (useLoopbackTransport)
            {
                AttachTransport(new ResidualVoiceLoopbackTransport());
            }
        }

        public void Connect()
        {
            EnsureClient();

            _client.Connect(sessionId, localPlayerId);

            if (_binding != null)
            {
                _binding.Start();
            }
        }

        public void Disconnect()
        {
            if (_binding != null)
            {
                _binding.Stop();
            }

            if (_client != null)
            {
                _client.Disconnect();
            }
        }

        public void AttachTransport(IResidualVoiceTransport transport)
        {
            if (transport == null)
            {
                throw new ArgumentNullException(nameof(transport));
            }

            if (_binding != null)
            {
                _binding.Dispose();
                _binding = null;
            }

            _transport = transport;
            _binding = new ResidualVoiceTransportBinding(_client, _transport);
        }

        public void Shutdown()
        {
            if (micInput != null)
            {
                micInput.StopCapture();
                micInput.DetachClient();
            }

            if (playbackSource != null)
            {
                playbackSource.DetachClient();
                playbackSource.Clear();
            }

            if (_binding != null)
            {
                _binding.Dispose();
                _binding = null;
            }

            _transport = null;

            if (_client != null)
            {
                if (logNativeMessages)
                {
                    _client.LogReceived -= OnNativeLogReceived;
                    _client.ErrorReceived -= OnNativeErrorReceived;
                }

                _client.Dispose();
                _client = null;
            }
        }

        private uint GetNowMs()
        {
            return (uint)(Time.realtimeSinceStartupAsDouble * 1000.0);
        }

        private void EnsureClient()
        {
            if (_client == null || !_client.IsCreated)
            {
                throw new InvalidOperationException("ResidualVoiceRig has not created a voice client yet.");
            }
        }

        private void OnNativeLogReceived(string message)
        {
            Debug.Log($"Residual Voice: {message}", this);
        }

        private void OnNativeErrorReceived(string message)
        {
            Debug.LogError($"Residual Voice: {message}", this);
        }
    }
}