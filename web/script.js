// Get elements
const statusDiv = document.getElementById('connection-status');
const buttonGrid = document.getElementById('button-grid');

let websocket = null;

// --- WebSocket Connection --- 
function connectWebSocket() {
    // Use the same host and port as the HTTP server, but with ws:// protocol
    const wsUrl = `ws://${window.location.host}`;
    console.log(`Attempting to connect to WebSocket: ${wsUrl}`);
    statusDiv.textContent = 'Connecting...';
    statusDiv.className = 'status-connecting'; // You can add styles for this

    websocket = new WebSocket(wsUrl);

    websocket.onopen = (event) => {
        console.log('WebSocket connection opened');
        statusDiv.textContent = 'Connected';
        statusDiv.className = 'status-connected';
        // Buttons will be loaded when the initial_config message is received
        // loadButtons(); // REMOVED: No longer load static buttons on open
    };

    websocket.onclose = (event) => {
        console.log('WebSocket connection closed:', event.code, event.reason);
        statusDiv.textContent = 'Disconnected. Attempting to reconnect...';
        statusDiv.className = 'status-disconnected';
        websocket = null;
        // Simple reconnect attempt after a delay
        setTimeout(connectWebSocket, 5000);
    };

    websocket.onerror = (event) => {
        console.error('WebSocket error:', event);
        statusDiv.textContent = 'Connection Error';
        statusDiv.className = 'status-error';
    };

    websocket.onmessage = (event) => {
        console.log('Message from server:', event.data);
        try {
            const message = JSON.parse(event.data);
            // Handle different message types from server
            if (message.type === 'initial_config') {
                console.log('Received initial configuration:', message.payload.layout);
                loadButtons(message.payload.layout);
            } else {
                 console.log('Received other message type:', message.type);
                 // Handle other potential message types here if needed
            }
        } catch (error) {
            console.error("Failed to parse message from server:", error);
        }
    };
}

// --- Button Handling --- 
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
        statusDiv.textContent = 'Error: Not Connected';
        statusDiv.className = 'status-error';
    }
}

// --- Load Buttons ---
// Loads buttons based on the layout provided by the server
function loadButtons(buttonLayout) {
    buttonGrid.innerHTML = ''; // Clear existing buttons

    if (!buttonLayout || buttonLayout.length === 0) {
        console.log("No button layout received or layout is empty.");
        const messageElement = document.createElement('p');
        messageElement.textContent = 'No buttons configured on the server.';
        messageElement.style.textAlign = 'center';
        messageElement.style.gridColumn = 'span 3'; // Span across grid if needed
        buttonGrid.appendChild(messageElement);
        return; // Exit if no buttons to display
    }

    // Create buttons based on the received layout
    buttonLayout.forEach(button => {
        if (!button.id || !button.name) {
            console.warn('Skipping button with missing id or name:', button);
            return; // Skip invalid button data
        }

        // --- ADDED for debugging ---
        console.log(`Processing button: ID=${button.id}, Name=${button.name}, IconPath='${button.icon_path}'`); 
        // ---------------------------

        const btnElement = document.createElement('button');
        btnElement.className = 'grid-button';
        btnElement.onclick = () => sendButtonPress(button.id);

        // --- ADDED: Icon Handling ---
        if (button.icon_path && button.icon_path.trim() !== '') {
             // --- ADDED for debugging ---
             console.log(` -> Icon path found for ${button.id}: ${button.icon_path}. Creating img element.`);
             // ---------------------------
            // If icon_path exists and is not empty
            btnElement.classList.add('has-icon'); // Add class for styling

            // Create image element
            const imgElement = document.createElement('img');
            imgElement.src = button.icon_path; // Set the source (relative path)
            imgElement.alt = button.name; // Use button name as alt text
            imgElement.onerror = () => {
                // Handle image loading errors (e.g., show text instead)
                console.error(`Failed to load icon: ${button.icon_path}`);
                imgElement.remove(); // Remove the broken image element
                // Optionally add text content back if image fails
                if (!btnElement.querySelector('.button-text')) { 
                     const textElementFallback = document.createElement('span');
                     textElementFallback.className = 'button-text';
                     textElementFallback.textContent = button.name;
                     btnElement.appendChild(textElementFallback);
                }
            };
            btnElement.appendChild(imgElement);

            // Add text label below/next to the icon 
            const textElement = document.createElement('span');
            textElement.className = 'button-text'; // Add class for styling
            textElement.textContent = button.name;
            btnElement.appendChild(textElement);

        } else {
             // --- ADDED for debugging ---
             console.log(` -> No valid icon path for ${button.id}. Using text only.`);
             // ---------------------------
            // No icon_path provided, just show text
            btnElement.textContent = button.name;
        }
        // --- END of Icon Handling ---

        buttonGrid.appendChild(btnElement);
    });
}

// --- Initial Load ---
document.addEventListener('DOMContentLoaded', () => {
    connectWebSocket();
}); 