using System;
using System.Collections.Generic;
using UnityEngine;

namespace Residual.Voice
{
    [DisallowMultipleComponent]
    public sealed class ResidualVoiceLocalPeerHub : MonoBehaviour
    {
        private readonly Dictionary<ushort, ResidualVoiceLocalPeerTransport> _peers =
            new Dictionary<ushort, ResidualVoiceLocalPeerTransport>();

        [SerializeField]
        private int maxPeers = 4;

        [SerializeField]
        private bool deliverToSender;

        [SerializeField]
        private bool logRouting;

        [Header("Runtime Debug")]
        [SerializeField]
        private int registeredPeerCountDebug;

        [SerializeField]
        private int routedPacketCountDebug;

        [SerializeField]
        private int deliveredPacketCountDebug;

        [SerializeField]
        private string lastRouteDebug = "No packets routed";

        public int MaxPeers
        {
            get => maxPeers;
            set => maxPeers = Mathf.Clamp(value, 1, 4);
        }

        public bool DeliverToSender
        {
            get => deliverToSender;
            set => deliverToSender = value;
        }

        public int RegisteredPeerCount => _peers.Count;

        private void OnValidate()
        {
            maxPeers = Mathf.Clamp(maxPeers, 1, 4);
        }

        private void Update()
        {
            registeredPeerCountDebug = _peers.Count;
        }

        public void Register(ResidualVoiceLocalPeerTransport peer)
        {
            if (peer == null)
            {
                throw new ArgumentNullException(nameof(peer));
            }

            var playerId = peer.LocalPlayerId;

            if (playerId == 0)
            {
                throw new InvalidOperationException("Residual voice local peer player ID must be greater than zero.");
            }

            if (!_peers.ContainsKey(playerId) && _peers.Count >= maxPeers)
            {
                throw new InvalidOperationException($"Residual voice local peer hub only supports {maxPeers} peers.");
            }

            if (_peers.TryGetValue(playerId, out var existing) && existing != peer)
            {
                throw new InvalidOperationException($"Residual voice local peer ID {playerId} is already registered.");
            }

            _peers[playerId] = peer;
            registeredPeerCountDebug = _peers.Count;
        }

        public void Unregister(ResidualVoiceLocalPeerTransport peer)
        {
            if (peer == null)
            {
                return;
            }

            var playerId = peer.LocalPlayerId;

            if (_peers.TryGetValue(playerId, out var existing) && existing == peer)
            {
                _peers.Remove(playerId);
            }

            registeredPeerCountDebug = _peers.Count;
        }

        public void RoutePacket(ResidualVoiceLocalPeerTransport sender, byte[] packet, int size)
        {
            if (sender == null)
            {
                throw new ArgumentNullException(nameof(sender));
            }

            if (packet == null)
            {
                throw new ArgumentNullException(nameof(packet));
            }

            if (size <= 0 || size > packet.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(size));
            }

            routedPacketCountDebug++;

            var delivered = 0;

            foreach (var pair in _peers)
            {
                var receiver = pair.Value;

                if (receiver == null || !receiver.IsRunning)
                {
                    continue;
                }

                if (!deliverToSender && receiver == sender)
                {
                    continue;
                }

                receiver.EnqueueIncoming(packet, size);
                delivered++;
            }

            deliveredPacketCountDebug += delivered;
            lastRouteDebug = $"Sender {sender.LocalPlayerId} routed {size} bytes to {delivered} peer(s).";

            if (logRouting)
            {
                Debug.Log($"ResidualVoiceLocalPeerHub: {lastRouteDebug}", this);
            }
        }
    }
}