using System;

namespace Residual.Voice
{
    public sealed class ResidualVoiceTransportBinding : IDisposable
    {
        private readonly ResidualVoiceClient _client;
        private readonly IResidualVoiceTransport _transport;

        private bool _disposed;
        private uint _nowMs;

        public ResidualVoiceTransportBinding(
            ResidualVoiceClient client,
            IResidualVoiceTransport transport)
        {
            _client = client ?? throw new ArgumentNullException(nameof(client));
            _transport = transport ?? throw new ArgumentNullException(nameof(transport));

            _client.PacketReady += OnPacketReady;
            _transport.PacketReceived += OnPacketReceived;
        }

        public void Start()
        {
            ThrowIfDisposed();
            _transport.Start();
        }

        public void Stop()
        {
            if (_disposed)
            {
                return;
            }

            _transport.Stop();
        }

        public void Tick(uint nowMs)
        {
            ThrowIfDisposed();

            _nowMs = nowMs;

            _transport.Tick();
            _client.Tick(nowMs);
        }

        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }

            _client.PacketReady -= OnPacketReady;
            _transport.PacketReceived -= OnPacketReceived;

            _transport.Dispose();

            _disposed = true;
        }

        private void OnPacketReady(byte[] packet, int size)
        {
            if (_disposed)
            {
                return;
            }

            _transport.Send(packet, size);
        }

        private void OnPacketReceived(byte[] packet, int size)
        {
            if (_disposed)
            {
                return;
            }

            _client.IngestPacket(packet, size, _nowMs);
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
            {
                throw new ObjectDisposedException(nameof(ResidualVoiceTransportBinding));
            }
        }
    }
}