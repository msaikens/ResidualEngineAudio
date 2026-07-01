using System;
using UnityEngine;

namespace Residual.Voice
{
    public static class ResidualVoicePcmUtility
    {
        private const float Pcm16ToFloatScale = 1.0f / 32768.0f;
        private const float FloatToPcm16Scale = 32767.0f;

        public static short FloatToPcm16(float sample)
        {
            sample = Mathf.Clamp(sample, -1.0f, 1.0f);
            return (short)Mathf.RoundToInt(sample * FloatToPcm16Scale);
        }

        public static float Pcm16ToFloat(short sample)
        {
            return sample * Pcm16ToFloatScale;
        }

        public static int ConvertFloatToPcm16(
            float[] source,
            int sourceOffset,
            short[] destination,
            int destinationOffset,
            int sampleCount)
        {
            ValidateArray(source, sourceOffset, sampleCount, nameof(source));
            ValidateArray(destination, destinationOffset, sampleCount, nameof(destination));

            for (var i = 0; i < sampleCount; i++)
            {
                destination[destinationOffset + i] = FloatToPcm16(source[sourceOffset + i]);
            }

            return sampleCount;
        }

        public static int ConvertPcm16ToFloat(
            short[] source,
            int sourceOffset,
            float[] destination,
            int destinationOffset,
            int sampleCount)
        {
            ValidateArray(source, sourceOffset, sampleCount, nameof(source));
            ValidateArray(destination, destinationOffset, sampleCount, nameof(destination));

            for (var i = 0; i < sampleCount; i++)
            {
                destination[destinationOffset + i] = Pcm16ToFloat(source[sourceOffset + i]);
            }

            return sampleCount;
        }

        public static int DownmixInterleavedFloatToMonoPcm16(
            float[] source,
            int sourceOffset,
            int sourceChannels,
            short[] destination,
            int destinationOffset,
            int frameCount)
        {
            if (sourceChannels <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(sourceChannels));
            }

            ValidateArray(source, sourceOffset, frameCount * sourceChannels, nameof(source));
            ValidateArray(destination, destinationOffset, frameCount, nameof(destination));

            for (var frame = 0; frame < frameCount; frame++)
            {
                var sum = 0.0f;
                var baseIndex = sourceOffset + frame * sourceChannels;

                for (var channel = 0; channel < sourceChannels; channel++)
                {
                    sum += source[baseIndex + channel];
                }

                destination[destinationOffset + frame] = FloatToPcm16(sum / sourceChannels);
            }

            return frameCount;
        }

        public static int UpmixMonoPcm16ToInterleavedFloat(
            short[] source,
            int sourceOffset,
            float[] destination,
            int destinationOffset,
            int destinationChannels,
            int frameCount)
        {
            if (destinationChannels <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(destinationChannels));
            }

            ValidateArray(source, sourceOffset, frameCount, nameof(source));
            ValidateArray(destination, destinationOffset, frameCount * destinationChannels, nameof(destination));

            for (var frame = 0; frame < frameCount; frame++)
            {
                var sample = Pcm16ToFloat(source[sourceOffset + frame]);
                var baseIndex = destinationOffset + frame * destinationChannels;

                for (var channel = 0; channel < destinationChannels; channel++)
                {
                    destination[baseIndex + channel] = sample;
                }
            }

            return frameCount;
        }

        public static int MixMonoPcm16IntoInterleavedFloat(
            short[] source,
            int sourceOffset,
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

            ValidateArray(source, sourceOffset, frameCount, nameof(source));
            ValidateArray(destination, destinationOffset, frameCount * destinationChannels, nameof(destination));

            for (var frame = 0; frame < frameCount; frame++)
            {
                var sample = Pcm16ToFloat(source[sourceOffset + frame]) * gain;
                var baseIndex = destinationOffset + frame * destinationChannels;

                for (var channel = 0; channel < destinationChannels; channel++)
                {
                    destination[baseIndex + channel] = Mathf.Clamp(
                        destination[baseIndex + channel] + sample,
                        -1.0f,
                        1.0f);
                }
            }

            return frameCount;
        }

        public static void Clear(float[] buffer)
        {
            if (buffer == null)
            {
                throw new ArgumentNullException(nameof(buffer));
            }

            Array.Clear(buffer, 0, buffer.Length);
        }

        public static void Clear(short[] buffer)
        {
            if (buffer == null)
            {
                throw new ArgumentNullException(nameof(buffer));
            }

            Array.Clear(buffer, 0, buffer.Length);
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