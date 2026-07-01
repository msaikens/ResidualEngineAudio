using System;

namespace Residual.Voice
{
    public interface IResidualVoiceTransport : IDisposable
    {
        event Action<byte[], int>? PacketReceived;

        bool IsRunning { get; }

        void Start();

        void Stop();

        void Send(byte[] packet, int size);

        void Tick();
    }
}