using UnityEngine;
using Residual.Voice;

public sealed class ResidualVoiceBasicLoopbackDemo : MonoBehaviour
{
    [Header("Session")]
    [SerializeField]
    private ulong sessionId = 12345;

    [SerializeField]
    private ushort localPlayerId = 1;

    [Header("Transmit")]
    [SerializeField]
    private bool alwaysTransmit = true;

    [SerializeField]
    private KeyCode pushToTalkKey = KeyCode.V;

    [Header("Microphone")]
    [SerializeField]
    private bool startMicrophoneOnStart = true;

    [Header("Debug")]
    [SerializeField]
    private bool showGui = true;

    [SerializeField]
    private bool logStatus = true;

    private ResidualVoiceRig _rig;
    private ResidualVoiceMicInput _micInput;
    private ResidualVoicePlaybackSource _playbackSource;

    private void Awake()
    {
        EnsureRequiredComponents();
    }

    private void Start()
    {
        if (logStatus)
        {
            Debug.Log("Residual Voice Basic Loopback sample started.", this);
            Debug.Log($"Residual Voice native API version: {ResidualVoiceClient.NativeApiVersion}", this);
        }

        if (startMicrophoneOnStart && _micInput != null && !_micInput.IsRecording)
        {
            _micInput.StartCapture();
        }
    }

    private void Update()
    {
        if (_rig == null || _rig.Client == null || !_rig.Client.IsCreated)
        {
            return;
        }

        var transmit = alwaysTransmit || Input.GetKey(pushToTalkKey);

        _rig.Client.SetLocalState(
            transform.position,
            transform.forward,
            pushToTalkDown: transmit,
            radioEnabled: false,
            radioChannel: 0);
    }

    private void OnGUI()
    {
        if (!showGui)
        {
            return;
        }

        GUILayout.BeginArea(new Rect(20, 20, 460, 260), GUI.skin.box);

        GUILayout.Label("Residual Voice - Basic Loopback");
        GUILayout.Label($"Session ID: {sessionId}");
        GUILayout.Label($"Local Player ID: {localPlayerId}");
        GUILayout.Label($"Native API: {ResidualVoiceClient.NativeApiVersion}");
        GUILayout.Label($"Rig running: {_rig != null && _rig.IsRunning}");
        GUILayout.Label($"Mic recording: {_micInput != null && _micInput.IsRecording}");
        GUILayout.Label($"Playback speakers: {(_playbackSource != null ? _playbackSource.SpeakerCount : 0)}");
        GUILayout.Label(alwaysTransmit
            ? "Transmit mode: Always transmitting"
            : $"Transmit mode: Hold {pushToTalkKey}");

        GUILayout.Space(8);

        if (GUILayout.Button("Start Microphone") && _micInput != null)
        {
            _micInput.StartCapture();
        }

        if (GUILayout.Button("Stop Microphone") && _micInput != null)
        {
            _micInput.StopCapture();
        }

        if (GUILayout.Button("Restart Microphone") && _micInput != null)
        {
            _micInput.RestartCapture();
        }

        alwaysTransmit = GUILayout.Toggle(alwaysTransmit, "Always transmit");

        GUILayout.EndArea();
    }

    private void EnsureRequiredComponents()
    {
        if (GetComponent<AudioSource>() == null)
        {
            gameObject.AddComponent<AudioSource>();
        }

        _micInput = GetComponent<ResidualVoiceMicInput>();
        if (_micInput == null)
        {
            _micInput = gameObject.AddComponent<ResidualVoiceMicInput>();
        }

        _playbackSource = GetComponent<ResidualVoicePlaybackSource>();
        if (_playbackSource == null)
        {
            _playbackSource = gameObject.AddComponent<ResidualVoicePlaybackSource>();
        }

        _rig = GetComponent<ResidualVoiceRig>();
        if (_rig == null)
        {
            _rig = gameObject.AddComponent<ResidualVoiceRig>();
        }
    }
}