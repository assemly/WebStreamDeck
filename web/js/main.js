// web/js/main.js

// Debounce function
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

document.addEventListener('DOMContentLoaded', () => {
    console.log("DOM fully loaded and parsed");

    // Ensure modules are loaded (simple global check for now)
    if (window.uiModule && window.websocketService && window.config) {
        console.log("Modules loaded, initializing...");
        
        // Initialize UI (e.g., attach fullscreen listener, internal resize handler)
        window.uiModule.init();

        // Initialize WebSocket connection, passing UI module and status element
        const statusDiv = document.getElementById('connection-status'); // Get status div here
        window.websocketService.connectWebSocket(window.uiModule, statusDiv); // Pass the element

        // REMOVED: Resize handling is now done internally within ui.js
        /*
        const reloadLayout = debounce(() => {
            console.log("Resize/orientation change detected, reloading layout...");
            // Previous logic attempted to call non-existent loadButtons
        }, 250);
        window.addEventListener('resize', reloadLayout);
        */

    } else {
        console.error("Required JavaScript modules not loaded!");
        // Display error to user?
        const statusDiv = document.getElementById('connection-status');
        if(statusDiv) {
            statusDiv.textContent = 'Initialization Error';
            statusDiv.className = 'status-error';
        }
    }

    // Potential: Add resize listener here, maybe calling a uiModule.handleResize() function
    // window.addEventListener('resize', () => uiModule.handleResize());
}); 