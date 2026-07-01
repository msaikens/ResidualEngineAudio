using System;

namespace Residual.Voice
{
    public sealed class ResidualVoicePlaybackBuffer
    {
        private readonly object _sync = new object();
        private readonly short[] _buffer;

        private int _readIndex;
        private int _writeIndex;
        private int _count;

        public ResidualVoicePlaybackBuffer(int capacitySamples = 48000)
        {
            if (capacitySamples <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(capacitySamples));
            }

            _buffer = new short[capacitySamples];
        }

        public int CapacitySamples => _buffer.Length;

        public int AvailableSamples
        {
            get
            {
                lock (_sync)
                {
                    return _count;
                }
            }
        }

        public bool DropOldestOnOverflow { get; set; } = true;

        public void Clear()
        {
            lock (_sync)
            {
                Array.Clear(_buffer, 0, _buffer.Length);
                _readIndex = 0;
                _writeIndex = 0;
                _count = 0;
            }
        }

        public int Enqueue(short[] samples)
        {
            if (samples == null)
            {
                throw new ArgumentNullException(nameof(samples));
            }

            return Enqueue(samples, 0, samples.Length);
        }

        public int Enqueue(short[] samples, int offset, int count)
        {
            ValidateArray(samples, offset, count, nameof(samples));

            if (count == 0)
            {
                return 0;
            }

            lock (_sync)
            {
                var written = 0;

                for (var i = 0; i < count; i++)
                {
                    if (_count == _buffer.Length)
                    {
                        if (!DropOldestOnOverflow)
                        {
                            break;
                        }

                        _readIndex = (_readIndex + 1) % _buffer.Length;
                        _count--;
                    }

                    _buffer[_writeIndex] = samples[offset + i];
                    _writeIndex = (_writeIndex + 1) % _buffer.Length;
                    _count++;
                    written++;
                }

                return written;
            }
        }

        public int Read(short[] destination, int offset, int count)
        {
            ValidateArray(destination, offset, count, nameof(destination));

            if (count == 0)
            {
                return 0;
            }

            lock (_sync)
            {
                var read = Math.Min(count, _count);

                for (var i = 0; i < read; i++)
                {
                    destination[offset + i] = _buffer[_readIndex];
                    _readIndex = (_readIndex + 1) % _buffer.Length;
                    _count--;
                }

                if (read < count)
                {
                    Array.Clear(destination, offset + read, count - read);
                }

                return read;
            }
        }

        public int ReadIntoInterleavedFloat(
            float[] destination,
            int destinationOffset,
            int destinationChannels,
            int frameCount)
        {
            if (destinationChannels <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(destinationChannels));
            }

            ValidateArray(
                destination,
                destinationOffset,
                frameCount * destinationChannels,
                nameof(destination));

            if (frameCount == 0)
            {
                return 0;
            }

            lock (_sync)
            {
                var framesRead = Math.Min(frameCount, _count);

                for (var frame = 0; frame < framesRead; frame++)
                {
                    var sample = ResidualVoicePcmUtility.Pcm16ToFloat(_buffer[_readIndex]);
                    _readIndex = (_readIndex + 1) % _buffer.Length;
                    _count--;

                    var baseIndex = destinationOffset + frame * destinationChannels;

                    for (var channel = 0; channel < destinationChannels; channel++)
                    {
                        destination[baseIndex + channel] = sample;
                    }
                }

                for (var frame = framesRead; frame < frameCount; frame++)
                {
                    var baseIndex = destinationOffset + frame * destinationChannels;

                    for (var channel = 0; channel < destinationChannels; channel++)
                    {
                        destination[baseIndex + channel] = 0.0f;
                    }
                }

                return framesRead;
            }
        }

        public int MixIntoInterleavedFloat(
            float[] destination,
            int destinationOffset,
            int destinationChannels,
            int frameCount,
            float gain = 1.0f)
        {
            if (destinationChannels <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(destinationChannels));
            }

            ValidateArray(
                destination,
                destinationOffset,
                frameCount * destinationChannels,
                nameof(destination));

            if (frameCount == 0)
            {
                return 0;
            }

            lock (_sync)
            {
                var framesRead = Math.Min(frameCount, _count);

                for (var frame = 0; frame < framesRead; frame++)
                {
                    var sample = ResidualVoicePcmUtility.Pcm16ToFloat(_buffer[_readIndex]) * gain;
                    _readIndex = (_readIndex + 1) % _buffer.Length;
                    _count--;

                    var baseIndex = destinationOffset + frame * destinationChannels;

                    for (var channel = 0; channel < destinationChannels; channel++)
                    {
                        var mixed = destination[baseIndex + channel] + sample;

                        if (mixed > 1.0f)
                        {
                            mixed = 1.0f;
                        }
                        else if (mixed < -1.0f)
                        {
                            mixed = -1.0f;
                        }

                        destination[baseIndex + channel] = mixed;
                    }
                }

                return framesRead;
            }
        }

        private static void ValidateArray<T>(
            T[] array,
            int offset,
            int count,
            string argumentName)
        {
            if (array == null)
            {
                throw new ArgumentNullException(argumentName);
            }

            if (offset < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(offset));
            }

            if (count < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(count));
            }

            if (offset + count > array.Length)
            {
                throw new ArgumentException(
                    "Offset and count exceed the array length.",
                    argumentName);
            }
        }
    }
}