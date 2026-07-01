using UnityEngine;
using Residual.Voice;

public sealed class ResidualVoiceBasicLoopbackDemo : MonoBehaviour
{
    [Header("Session")]
    [SerializeField]
    private ulong sessionId = 12345;

    [SerializeField]
    private ushort localPlayerId = 1;

    [Header("Input")]
    [SerializeField]
    private bool startMicrophoneOnStart = true;

    [SerializeField]
    private bool alwaysTransmit = true;

    [Header("Runtime")]
    [SerializeField]
    private bool logStatus = true;

    private ResidualVoiceRig _rig;
    private ResidualVoiceMicInput _micInput;
    private ResidualVoicePlaybackSource _playbackSource;
    private ResidualVoicePushToTalk _pushToTalk;

    private void Awake()
    {
        _rig = GetOrAdd<ResidualVoiceRig>();
        _micInput = GetOrAdd<ResidualVoiceMicInput>();
        _playbackSource = GetOrAdd<ResidualVoicePlaybackSource>();
        _pushToTalk = GetOrAdd<ResidualVoicePushToTalk>();

        if (GetComponent<AudioSource>() == null)
        {
            gameObject.AddComponent<AudioSource>();
        }
    }

    private void Start()
    {
        if (logStatus)
        {
            Debug.Log("Residual Voice Basic Loopback Demo starting.", this);
            Debug.Log($"Native API version: {ResidualVoiceClient.NativeApiVersion}", this);
        }

        _rig.SendMessage("Create", SendMessageOptions.DontRequireReceiver);
        _rig.SendMessage("Connect", SendMessageOptions.DontRequireReceiver);

        if (startMicrophoneOnStart)
        {
            _micInput.StartCapture();
        }
    }

    private void OnGUI()
    {
        GUILayout.BeginArea(new Rect(20, 20, 420, 220), GUI.skin.box);

        GUILayout.Label("Residual Voice - Basic Loopback Demo");
        GUILayout.Label($"Session: {sessionId}");
        GUILayout.Label($"Player: {localPlayerId}");
        GUILayout.Label($"Mic recording: {_micInput != null && _micInput.IsRecording}");
        GUILayout.Label($"Rig running: {_rig != null && _rig.IsRunning}");

        if (GUILayout.Button("Start Microphone"))
        {
            _micInput.StartCapture();
        }

        if (GUILayout.Button("Stop Microphone"))
        {
            _micInput.StopCapture();
        }

        if (GUILayout.Button("Restart Microphone"))
        {
            _micInput.RestartCapture();
        }

        GUILayout.EndArea();
    }

    private static T GetOrAdd<T>() where T : Component
    {
        var existing = FindFirstObjectByType<T>();

        if (existing != null)
        {
            return existing;
        }

        var host = new GameObject(typeof(T).Name);
        return host.AddComponent<T>();
    }
}