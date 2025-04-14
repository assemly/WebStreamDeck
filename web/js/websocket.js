// web/js/websocket.js

let websocket = null;
let uiModule = null; // Will be injected
let connectionStatusDiv = null; // Direct reference to status element

function connectWebSocket(appUIModule, statusElement) {
    uiModule = appUIModule;
    connectionStatusDiv = statusElement;

    console.log(`Attempting to connect to WebSocket: ${config.websocketUrl}`);
    updateStatus('Connecting...', 'status-connecting');

    websocket = new WebSocket(config.websocketUrl);

    websocket.onopen = (event) => {
        console.log('WebSocket connection opened');
        updateStatus('Connected', 'status-connected');
        // Request initial config or state if needed
        // Example: websocket.send(JSON.stringify({ type: 'get_config' }));
    };

    websocket.onclose = (event) => {
        console.log('WebSocket connection closed:', event.code, event.reason);
        updateStatus('Disconnected. Reconnecting...', 'status-disconnected');
        websocket = null;
        // Simple reconnect attempt
        setTimeout(() => connectWebSocket(uiModule, connectionStatusDiv), 5000);
    };

    websocket.onerror = (event) => {
        console.error('WebSocket error:', event);
        updateStatus('Connection Error', 'status-error');
    };

    websocket.onmessage = (event) => {
        console.log('Message from server:', event.data);
        try {
            const message = JSON.parse(event.data);
            handleServerMessage(message);
        } catch (error) {
            console.error("Failed to parse message from server:", error);
        }
    };
}

function handleServerMessage(message) {
    if (!uiModule) return; // Guard against UI module not ready

    switch (message.type) {
        case 'initial_config':
        case 'layout_update': // Assuming server might send updates
            console.log('Received layout configuration:', message.payload.layout);
            uiModule.loadButtons(message.payload.layout);
            break;
        // Add other message types here
        default:
            console.log('Received unhandled message type:', message.type);
    }
}

function sendButtonPress(buttonId) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        const message = {
            type: 'button_press',
            payload: {
                button_id: buttonId
            }
        };
        websocket.send(JSON.stringify(message));
        console.log(`Sent button press: ${buttonId}`);
    } else {
        console.error('WebSocket is not connected.');
        updateStatus('Error: Not Connected', 'status-error');
    }
}

function updateStatus(text, className) {
    if (connectionStatusDiv) {
        connectionStatusDiv.textContent = text;
        connectionStatusDiv.className = className;
    }
}

// Export functions needed by other modules
window.websocketService = {
    connectWebSocket,
    sendButtonPress
}; 