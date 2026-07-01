using System;
using System.Collections.Generic;
using UnityEngine;

namespace Residual.Voice
{
    [DisallowMultipleComponent]
    public sealed class ResidualVoiceLocalPeerTransport : MonoBehaviour, IResidualVoiceTransport
    {
        private readonly Queue<IncomingPacket> _incomingPackets = new Queue<IncomingPacket>();

        private bool _isRunning;

        [SerializeField]
        private ResidualVoiceLocalPeerHub hub;

        [SerializeField]
        private ushort localPlayerId = 1;

        [SerializeField]
        private bool autoFindHub = true;

        [SerializeField]
        private bool registerOnEnable = true;

        [SerializeField]
        private int maxQueuedPackets = 256;

        [SerializeField]
        private bool logPackets;

        [Header("Runtime Debug")]
        [SerializeField]
        private bool isRunningDebug;

        [SerializeField]
        private int queuedPacketCountDebug;

        [SerializeField]
        private int sentPacketCountDebug;

        [SerializeField]
        private int receivedPacketCountDebug;

        [SerializeField]
        private int deliveredPacketCountDebug;

        [SerializeField]
        private string lastStatusDebug = "Not started";

        public event Action<byte[], int> PacketReceived;

        public bool IsRunning => _isRunning;

        public ushort LocalPlayerId
        {
            get => localPlayerId;
            set
            {
                if (value == 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(value), "Player ID must be greater than zero.");
                }

                if (_isRunning)
                {
                    throw new InvalidOperationException("Cannot change local player ID while transport is running.");
                }

                localPlayerId = value;
            }
        }

        public ResidualVoiceLocalPeerHub Hub
        {
            get => hub;
            set
            {
                if (_isRunning)
                {
                    throw new InvalidOperationException("Cannot change hub while transport is running.");
                }

                hub = value;
            }
        }

        private void OnEnable()
        {
            if (registerOnEnable)
            {
                StartTransport();
            }
        }

        private void OnDisable()
        {
            StopTransport();
        }

        private void OnDestroy()
        {
            Dispose();
        }

        private void OnValidate()
        {
            if (localPlayerId == 0)
            {
                localPlayerId = 1;
            }

            if (maxQueuedPackets <= 0)
            {
                maxQueuedPackets = 256;
            }
        }

        void IResidualVoiceTransport.Start()
        {
            StartTransport();
        }

        void IResidualVoiceTransport.Stop()
        {
            StopTransport();
        }

        void IResidualVoiceTransport.Send(byte[] packet, int size)
        {
            SendPacket(packet, size);
        }

        void IResidualVoiceTransport.Tick()
        {
            TickTransport();
        }

        public void Dispose()
        {
            StopTransport();

            lock (_incomingPackets)
            {
                _incomingPackets.Clear();
            }

            PacketReceived = null;
        }

        public void StartTransport()
        {
            if (_isRunning)
            {
                return;
            }

            ResolveHub();

            if (hub == null)
            {
                lastStatusDebug = "No local peer hub found";
                throw new InvalidOperationException("ResidualVoiceLocalPeerTransport requires a ResidualVoiceLocalPeerHub.");
            }

            hub.Register(this);
            _isRunning = true;

            isRunningDebug = true;
            lastStatusDebug = $"Running as peer {localPlayerId}";
        }

        public void StopTransport()
        {
            if (!_isRunning)
            {
                return;
            }

            if (hub != null)
            {
                hub.Unregister(this);
            }

            _isRunning = false;
            isRunningDebug = false;
            lastStatusDebug = "Stopped";
        }

        public void SendPacket(byte[] packet, int size)
        {
            if (!_isRunning)
            {
                return;
            }

            if (packet == null)
            {
                throw new ArgumentNullException(nameof(packet));
            }

            if (size <= 0 || size > packet.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(size));
            }

            sentPacketCountDebug++;

            if (logPackets)
            {
                Debug.Log($"ResidualVoiceLocalPeerTransport peer {localPlayerId} sending {size} bytes.", this);
            }

            hub.RoutePacket(this, packet, size);
        }

        public void EnqueueIncoming(byte[] packet, int size)
        {
            if (packet == null)
            {
                throw new ArgumentNullException(nameof(packet));
            }

            if (size <= 0 || size > packet.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(size));
            }

            var copy = new byte[size];
            Array.Copy(packet, copy, size);

            lock (_incomingPackets)
            {
                while (_incomingPackets.Count >= maxQueuedPackets)
                {
                    _incomingPackets.Dequeue();
                }

                _incomingPackets.Enqueue(new IncomingPacket(copy, size));
                queuedPacketCountDebug = _incomingPackets.Count;
            }

            receivedPacketCountDebug++;
        }

        public void TickTransport()
        {
            if (!_isRunning)
            {
                return;
            }

            while (true)
            {
                IncomingPacket packet;

                lock (_incomingPackets)
                {
                    if (_incomingPackets.Count == 0)
                    {
                        queuedPacketCountDebug = 0;
                        return;
                    }

                    packet = _incomingPackets.Dequeue();
                    queuedPacketCountDebug = _incomingPackets.Count;
                }

                deliveredPacketCountDebug++;
                PacketReceived?.Invoke(packet.Data, packet.Size);
            }
        }

        private void ResolveHub()
        {
            if (hub != null || !autoFindHub)
            {
                return;
            }

#if UNITY_2023_1_OR_NEWER
            hub = FindFirstObjectByType<ResidualVoiceLocalPeerHub>();
#else
            hub = FindObjectOfType<ResidualVoiceLocalPeerHub>();
#endif
        }

        private readonly struct IncomingPacket
        {
            public IncomingPacket(byte[] data, int size)
            {
                Data = data;
                Size = size;
            }

            public byte[] Data { get; }
            public int Size { get; }
        }
    }
}