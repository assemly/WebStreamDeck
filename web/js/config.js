// web/js/config.js

const config = {
    buttonsPerPage: 18, // 6x3 layout for pagination
    websocketUrl: `ws://${window.location.host}` // Calculate WebSocket URL
};

// Expose config globally
window.config = config; 