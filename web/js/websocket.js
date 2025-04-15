// web/js/websocket.js

let websocket = null;
let uiModule = null; // Will be injected
let connectionStatusDiv = null; // Direct reference to status element

function connectWebSocket(appUIModule, statusElement) {
    uiModule = appUIModule;
    connectionStatusDiv = statusElement;

    const host = config.serverAddress || window.location.hostname;
    const port = config.serverPort || 9002;
    const url = `ws://${host}:${port}/ws`;
    console.log(`Attempting to connect to WebSocket: ${url}`);
    updateStatus('Connecting...', 'status-connecting');

    websocket = new WebSocket(url);

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
        case 'initial_state':
            if (message.payload && message.payload.buttons && message.payload.layout) {
                console.log('Received initial state:', message.payload);
                uiModule.loadInitialData(message.payload.buttons, message.payload.layout);
            } else {
                console.error('Invalid initial_state payload received:', message.payload);
            }
            break;
        case 'layout_update':
            if (message.payload && message.payload.layout) {
                console.log('Received layout update:', message.payload.layout);
                console.warn('Received layout_update, but full reload logic might be needed.');
            } else {
                console.error('Invalid layout_update payload received:', message.payload);
            }
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