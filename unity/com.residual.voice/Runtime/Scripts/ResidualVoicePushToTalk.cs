using UnityEngine;

namespace Residual.Voice
{
    [DisallowMultipleComponent]
    public sealed class ResidualVoicePushToTalk : MonoBehaviour
    {
        [SerializeField]
        private ResidualVoiceRig rig;

        [SerializeField]
        private Transform trackedTransform;

        [SerializeField]
        private KeyCode proximityPushToTalkKey = KeyCode.V;

        [SerializeField]
        private KeyCode radioPushToTalkKey = KeyCode.B;

        [SerializeField]
        private bool alwaysTransmitProximity;

        [SerializeField]
        private bool enableRadio;

        [SerializeField]
        [Range(0, 15)]
        private int radioChannel;

        [SerializeField]
        private bool useRadioKey;

        public bool IsProximityTransmitting { get; private set; }

        public bool IsRadioTransmitting { get; private set; }

        public int RadioChannel
        {
            get => radioChannel;
            set => radioChannel = Mathf.Clamp(value, 0, 15);
        }

        private void Awake()
        {
            if (rig == null)
            {
                rig = GetComponent<ResidualVoiceRig>();
            }

            if (trackedTransform == null)
            {
                trackedTransform = transform;
            }
        }

        private void Update()
        {
            if (rig == null || rig.Client == null || !rig.Client.IsCreated)
            {
                return;
            }

            var proximityDown = alwaysTransmitProximity || Input.GetKey(proximityPushToTalkKey);
            var radioDown = enableRadio && useRadioKey && Input.GetKey(radioPushToTalkKey);

            IsProximityTransmitting = proximityDown;
            IsRadioTransmitting = radioDown;

            var pttDown = proximityDown || radioDown;
            var radioEnabled = enableRadio && radioDown;

            rig.Client.SetLocalState(
                trackedTransform.position,
                trackedTransform.forward,
                pttDown,
                radioEnabled,
                (byte)radioChannel);
        }
    }
}