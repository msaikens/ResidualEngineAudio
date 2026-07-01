using System;
using System.Collections.Generic;

namespace Residual.Voice
{
    public sealed class ResidualVoiceLoopbackTransport : IResidualVoiceTransport
    {
        private readonly Queue<byte[]> _pendingPackets = new Queue<byte[]>();

        public event Action<byte[], int>? PacketReceived;

        public bool IsRunning { get; private set; }

        public bool DeliverImmediately { get; set; }

        public int MaxQueuedPackets { get; set; } = 256;

        public void Start()
        {
            IsRunning = true;
        }

        public void Stop()
        {
            IsRunning = false;
            _pendingPackets.Clear();
        }

        public void Send(byte[] packet, int size)
        {
            if (!IsRunning)
            {
                return;
            }

            if (packet == null)
            {
                throw new ArgumentNullException(nameof(packet));
            }

            if (size < 0 || size > packet.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(size));
            }

            var copy = new byte[size];
            Buffer.BlockCopy(packet, 0, copy, 0, size);

            if (DeliverImmediately)
            {
                PacketReceived?.Invoke(copy, copy.Length);
                return;
            }

            while (_pendingPackets.Count >= MaxQueuedPackets)
            {
                _pendingPackets.Dequeue();
            }

            _pendingPackets.Enqueue(copy);
        }

        public void Tick()
        {
            if (!IsRunning)
            {
                return;
            }

            while (_pendingPackets.Count > 0)
            {
                var packet = _pendingPackets.Dequeue();
                PacketReceived?.Invoke(packet, packet.Length);
            }
        }

        public void Dispose()
        {
            Stop();
        }
    }
}