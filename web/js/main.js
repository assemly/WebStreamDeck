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
        
        // Initialize UI (e.g., attach fullscreen listener)
        window.uiModule.init();

        // Initialize WebSocket connection, passing UI module and status element
        window.websocketService.connectWebSocket(window.uiModule, window.uiModule.connectionStatusDiv);

        // Create a debounced version of the function to reload layout
        const reloadLayout = debounce(() => {
            console.log("Resize/orientation change detected, reloading layout...");
            // Need access to the stored layout data. 
            // We assume uiModule internally stores it now.
            // We just need to call loadButtons with *something* to trigger the reload.
            // Passing null or undefined might be better if loadButtons handles it.
            // OR retrieve it if uiModule exposes it.
            
            // Simplest approach: Call loadButtons with the currently stored layout
            // This assumes uiModule.loadButtons can be called without arguments
            // and will use its internal `currentButtonLayout`.
            // Let's modify ui.js loadButtons to accept no arguments for this.
            window.uiModule.loadButtons(); // Trigger reload using internal data
            
        }, 250); // Wait 250ms after resize stops

        // Listen for resize events (includes orientation changes)
        window.addEventListener('resize', reloadLayout);

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