// web/js/ui.js

const ui = (() => {
    // Cache DOM elements
    const buttonGrid = document.getElementById('button-grid');
    const appTitle = document.getElementById('app-title');
    const paginationDotsContainer = document.getElementById('pagination-dots');
    const connectionStatusDiv = document.getElementById('connection-status');

    let currentPageIndex = 0;
    let currentLayout = null; // Store the entire layout object
    let buttonsById = {};     // Store buttons indexed by ID for quick lookup
    let isPortraitMode = false; // Track orientation

    // Touch swipe state variables
    let touchStartX = 0;
    let touchMoveX = 0;
    let isSwiping = false;
    const swipeThreshold = 50; // Minimum pixels to be considered a swipe

    // --- Initialization Function --- 
    function loadInitialData(buttons = [], layout = null) {
        console.log("UI: Loading initial data...");
        currentLayout = layout;
        // Convert button array to an object indexed by ID
        buttonsById = buttons.reduce((map, btn) => {
            map[btn.id] = btn;
            return map;
        }, {});

        // Reset common UI state
        currentPageIndex = 0;
        updateUILayout(); // Call central layout update function
    }

    // --- Central Layout Update Function ---
    function updateUILayout() {
        console.log("UI: Updating layout based on orientation...");
        isPortraitMode = window.matchMedia("(orientation: portrait)").matches;
        console.log(`UI: Orientation is ${isPortraitMode ? 'Portrait' : 'Landscape'}`);

        // --- Calculate Available Height for Button Grid ---
        let availableHeight = window.innerHeight;
        const headerElement = document.querySelector('header'); // <<< CHANGED: Select by tag name
        const paginationElement = document.getElementById('pagination-dots');
        let paginationHeight = 0;

        if (headerElement) {
            availableHeight -= headerElement.offsetHeight;
            // console.log(`Header height: ${headerElement.offsetHeight}`);
        } else {
            console.warn("UI: Header element (<header>) not found for height calculation."); // Updated warning message
        }

        // Check if pagination should be displayed (landscape, multiple pages)
        const needsPagination = !isPortraitMode && currentLayout && currentLayout.page_count > 1;

        if (needsPagination && paginationElement) {
            // Temporarily ensure pagination is visible to measure its height accurately
            const originalDisplay = paginationElement.style.display;
            paginationElement.style.display = 'flex'; // Or its default visible display style
            paginationHeight = paginationElement.offsetHeight;
            paginationElement.style.display = originalDisplay; // Restore original display (setupPagination will handle final state)
            // console.log(`Pagination height: ${paginationHeight}`);
        } else if (paginationElement) {
             paginationElement.style.display = 'none'; // Ensure it's hidden if not needed
        }

        availableHeight -= paginationHeight;

        // Account for grid padding (adjust value as needed or remove if using border-box)
        const gridPaddingTop = 8;    // Example padding value from CSS
        const gridPaddingBottom = 8; // Example padding value from CSS
        availableHeight -= (gridPaddingTop + gridPaddingBottom);

        // console.log(`Calculated available height for grid: ${availableHeight}`);

        // Reset grid before rendering
        buttonGrid.innerHTML = '';
        buttonGrid.className = 'button-grid'; // Base class
        buttonGrid.style.cssText = ''; // Reset inline styles specifically for height override later
        // Set the calculated height
        if (availableHeight > 0) {
            buttonGrid.style.height = `${availableHeight}px`;
             console.log(`UI: Set button-grid height to ${availableHeight}px`);
        } else {
            console.warn("UI: Calculated available height for grid is zero or negative.");
            buttonGrid.style.height = 'auto'; // Fallback
        }
        // Reset pagination display (will be set correctly later if needed)
        paginationDotsContainer.innerHTML = '';
        paginationDotsContainer.style.display = 'none';

        removeSwipeListeners(); // Remove listeners by default


        if (!currentLayout || !buttonsById) {
             console.error("UI: Missing layout or button data for rendering.");
             buttonGrid.innerHTML = 'Error: Missing data.';
             return;
        }


        if (isPortraitMode) {
            renderPortraitGrid();
            // No pagination or swipe in portrait mode
        } else {
            renderLandscapeGrid(); // Use the dynamic layout for landscape
            // Setup pagination/swipe only if needed in landscape
            if (needsPagination) { // Use the flag calculated earlier
                setupPagination(currentLayout.page_count); // setupPagination will set display='flex' again
                addSwipeListeners();
            }
        }
    }


    // --- Portrait Grid Rendering ---
    function renderPortraitGrid() {
        console.log("UI: Rendering Portrait Grid (3 columns)");
        buttonGrid.innerHTML = ''; // Clear previous buttons

        // --- MODIFIED: Collect buttons based on layout order across all pages ---
        const buttonsToRender = [];
        if (!currentLayout || !currentLayout.pages || !Array.isArray(currentLayout.pages)) { // Added Array check
            console.error("UI: Layout pages data is missing or not an array for portrait rendering.");
            return; // Exit if layout is not available or invalid
        }
        
        // <<< MODIFIED: Correctly iterate over the array of [pageIndex, pageGrid] pairs >>>
        // Sort the pages array based on the page index (first element of each pair)
        const sortedPages = [...currentLayout.pages].sort((a, b) => a[0] - b[0]);

        // Iterate through the sorted page entries
        for (const pageEntry of sortedPages) {
            const pageIndex = pageEntry[0]; // Optional: can use this in logs if needed
            const pageLayout = pageEntry[1]; // Get the actual grid
            if (!pageLayout) continue; // Skip if grid data is missing

            for (let r = 0; r < pageLayout.length; r++) { // Iterate rows of the grid
                const rowLayout = pageLayout[r];
                if (!rowLayout) continue; // Skip if row data is missing

                for (let c = 0; c < rowLayout.length; c++) { // Iterate columns of the row
                    const buttonId = rowLayout[c]; // Get the button ID
                    // Check if buttonId exists, is a string, and is not empty after trimming
                    if (buttonId && typeof buttonId === 'string' && buttonId.trim() !== "") { 
                        const buttonData = buttonsById[buttonId];
                        if (buttonData) {
                            buttonsToRender.push(buttonData); // Add the button data if found
                        } else {
                            console.warn(`UI: Button ID '${buttonId}' found in layout page ${pageIndex}, but not in button definitions.`);
                        }
                    }
                }
            }
        }
        // <<< END OF MODIFICATION >>>

        console.log(`UI: Collected ${buttonsToRender.length} buttons from layout for portrait view.`);

        if (buttonsToRender.length === 0) {
            console.log("UI: No buttons found in layout to render for portrait view.");
            buttonGrid.style.display = 'none'; // Hide grid if empty
            return;
        }

        // const allButtons = Object.values(buttonsById); // Get all button data - OLD WAY
        // const totalButtons = allButtons.length;       // OLD WAY
        const totalButtons = buttonsToRender.length;   // NEW WAY
        const cols = 3;
        const rows = Math.ceil(totalButtons / cols);

        buttonGrid.style.setProperty('--grid-rows', rows);
        buttonGrid.style.setProperty('--grid-cols', cols);
        buttonGrid.style.display = 'grid';
        buttonGrid.style.gridTemplateRows = `repeat(${rows}, auto)`; // Auto height for rows
        buttonGrid.style.gridTemplateColumns = `repeat(${cols}, 1fr)`;
        buttonGrid.classList.remove('paged-grid'); // Ensure paged class is removed
        buttonGrid.classList.add('portrait-grid'); // Add specific class if needed

        // --- MODIFIED: Iterate over buttonsToRender --- 
        buttonsToRender.forEach(buttonData => {
            const cell = document.createElement('div');
            cell.className = 'grid-cell';
            const btnElement = createButtonElement(buttonData);
            cell.appendChild(btnElement);
            buttonGrid.appendChild(cell);
        });
        console.log(`UI: Finished rendering portrait grid (${rows}x${cols})`);
    }


    // --- Landscape Grid Rendering (Dynamic Layout) ---
    function renderLandscapeGrid() {
         if (!currentLayout || !buttonsById || !currentLayout.pages || !Array.isArray(currentLayout.pages)) {
            console.error(`UI: Cannot render landscape page ${currentPageIndex}, invalid layout or buttons data.`);
            buttonGrid.innerHTML = `Error: Invalid layout data for landscape.`;
            return;
        }

        const pagesArray = currentLayout.pages;
        const pageDataEntry = pagesArray.find(entry => entry && entry[0] === currentPageIndex);
        const pageLayout = pageDataEntry ? pageDataEntry[1] : null;

        if (!pageLayout) {
            console.error(`UI: Could not find layout data for page index ${currentPageIndex} in pages array.`);
            buttonGrid.innerHTML = `Error: Layout for page ${currentPageIndex} not found.`;
            return;
        }

        console.log(`UI: Rendering landscape layout for page ${currentPageIndex}`);
        buttonGrid.innerHTML = ''; // Clear previous content of grid

        const { rows_per_page, cols_per_page } = currentLayout;

        buttonGrid.style.setProperty('--grid-rows', rows_per_page);
        buttonGrid.style.setProperty('--grid-cols', cols_per_page);
        buttonGrid.style.display = 'grid';
        buttonGrid.style.gridTemplateRows = `repeat(${rows_per_page}, 1fr)`;
        buttonGrid.style.gridTemplateColumns = `repeat(${cols_per_page}, 1fr)`;
        buttonGrid.classList.remove('portrait-grid'); // Ensure portrait class is removed

        for (let r = 0; r < rows_per_page; r++) {
            for (let c = 0; c < cols_per_page; c++) {
                const cell = document.createElement('div');
                cell.className = 'grid-cell';
                const buttonId = pageLayout[r]?.[c];
                if (buttonId && buttonsById[buttonId]) {
                    const buttonData = buttonsById[buttonId];
                    const btnElement = createButtonElement(buttonData);
                    cell.appendChild(btnElement);
                }
                buttonGrid.appendChild(cell);
            }
        }
        console.log(`UI: Finished rendering landscape page ${currentPageIndex}`);
    }

    // --- Helper: Create Button Element ---
    function createButtonElement(buttonData) {
        const btnElement = document.createElement('button');
        btnElement.className = 'grid-button';
        btnElement.dataset.buttonId = buttonData.id; // Store ID for potential use
        btnElement.onclick = () => window.websocketService.sendButtonPress(buttonData.id);

        // Handle Icon Path - Ensure it starts with '/' if relative
        let iconPath = buttonData.icon_path || '';
        if (iconPath && !iconPath.startsWith('/') && !iconPath.startsWith('http')) {
             // Basic check to prepend slash if it looks relative to assets
             if(iconPath.startsWith('assets/')) {
                 iconPath = '/' + iconPath;
             }
             // Add more robust path handling if necessary
        }

        if (iconPath) {
            btnElement.classList.add('has-icon');
            const imgElement = document.createElement('img');
            imgElement.src = iconPath;
            imgElement.alt = buttonData.name;
            imgElement.onerror = () => {
                console.error(`Failed to load icon: ${iconPath}`);
                imgElement.remove(); // Remove broken image icon
                // Ensure text is visible if icon fails
                if (!btnElement.querySelector('.button-text')) {
                    const textElementFallback = document.createElement('span');
                    textElementFallback.className = 'button-text';
                    textElementFallback.textContent = buttonData.name;
                    btnElement.appendChild(textElementFallback);
                }
            };
            btnElement.appendChild(imgElement);

            // Add text label below/beside icon (adjust CSS as needed)
            const textElement = document.createElement('span');
            textElement.className = 'button-text';
            textElement.textContent = buttonData.name;
            btnElement.appendChild(textElement);

        } else {
            // No icon, just text
            const textElement = document.createElement('span');
            textElement.className = 'button-text';
            textElement.textContent = buttonData.name;
            btnElement.appendChild(textElement);
        }
        return btnElement;
    }

    // --- Pagination Setup ---
    function setupPagination(totalPages) {
        paginationDotsContainer.innerHTML = ''; // Clear existing dots
        if (totalPages > 1) {
            paginationDotsContainer.style.display = 'flex';
            for (let i = 0; i < totalPages; i++) {
                const dot = document.createElement('span');
                dot.className = 'dot';
                dot.dataset.pageIndex = i;
                dot.onclick = () => goToPage(i);
                paginationDotsContainer.appendChild(dot);
            }
            updatePaginationDots();
        } else {
            paginationDotsContainer.style.display = 'none';
        }
    }


    // --- Touch Swipe Handling --- 
    function addSwipeListeners() {
        // console.log("Adding swipe listeners");
        buttonGrid.addEventListener('touchstart', handleTouchStart, { passive: true });
        buttonGrid.addEventListener('touchmove', handleTouchMove, { passive: false });
        buttonGrid.addEventListener('touchend', handleTouchEnd, { passive: true });
     }
    function removeSwipeListeners() {
        // console.log("Removing swipe listeners");
        buttonGrid.removeEventListener('touchstart', handleTouchStart);
        buttonGrid.removeEventListener('touchmove', handleTouchMove);
        buttonGrid.removeEventListener('touchend', handleTouchEnd);
     }
    function handleTouchStart(event) {
        if (event.touches.length > 1 || !currentLayout || currentLayout.page_count <= 1 || isPortraitMode) return; // Added isPortraitMode check
        touchStartX = event.touches[0].clientX;
        touchMoveX = touchStartX;
        isSwiping = false;
     }
    function handleTouchMove(event) {
        if (event.touches.length > 1 || !currentLayout || currentLayout.page_count <= 1 || isPortraitMode) return; // Added isPortraitMode check
        touchMoveX = event.touches[0].clientX;
        const deltaX = touchMoveX - touchStartX;
        if (Math.abs(deltaX) > 10 && !isSwiping) {
            isSwiping = true;
        }
        if (isSwiping) {
            event.preventDefault();
        }
     }
    function handleTouchEnd(event) {
        if (!currentLayout || currentLayout.page_count <= 1 || !isSwiping || isPortraitMode) { // Added isPortraitMode check
            resetSwipeState();
            return;
        }
        const deltaX = touchMoveX - touchStartX;
        const totalPages = currentLayout.page_count;
        if (deltaX < -swipeThreshold) {
            goToPage(Math.min(currentPageIndex + 1, totalPages - 1));
        } else if (deltaX > swipeThreshold) {
            goToPage(Math.max(currentPageIndex - 1, 0));
        }
        resetSwipeState();
     }
    function resetSwipeState() {
        touchStartX = 0;
        touchMoveX = 0;
        isSwiping = false;
     }

    // --- Page Navigation --- 
    function goToPage(pageIndex) {
        if (isPortraitMode || !currentLayout) return; // Only allow page change in landscape
        const totalPages = currentLayout.page_count;

        if (pageIndex < 0 || pageIndex >= totalPages || pageIndex === currentPageIndex) {
            return;
        }

        console.log(`UI: Changing page from ${currentPageIndex} to ${pageIndex}`);
        currentPageIndex = pageIndex;
        renderLandscapeGrid(); // Call landscape renderer
        updatePaginationDots();
    }

    // --- Update Pagination Dots ---
    function updatePaginationDots() {
        const dots = paginationDotsContainer.querySelectorAll('.dot');
        dots.forEach((dot, index) => {
            dot.classList.toggle('active', index === currentPageIndex);
        });
     }

    // --- Full Screen ---
    function toggleFullScreen() {
        if (!document.fullscreenElement) {
            document.documentElement.requestFullscreen().catch(err => {
                alert(`Error attempting to enable full-screen mode: ${err.message} (${err.name})`);
            });
        } else {
            if (document.exitFullscreen) {
                document.exitFullscreen();
            }
        }
    }

    // --- Initialization ---
    function init() {
        console.log("UI Module Initialized");

        // <<< ADDED: Add click listener for the title to toggle fullscreen >>>
        const appTitleElement = document.getElementById('app-title'); // Assuming the title has id="app-title"
        if (appTitleElement) {
            appTitleElement.addEventListener('click', toggleFullScreen);
            console.log("UI: Added fullscreen toggle listener to title.");
        } else {
            console.warn("UI: Could not find element with id 'app-title' to attach fullscreen toggle.");
        }
        // <<< END OF ADDED CODE >>>

        // Use a library or more robust check if needed
        const debouncedLayoutUpdate = debounce(updateUILayout, 250);
        window.addEventListener('resize', debouncedLayoutUpdate);
        // Use orientationchange where available, fallback to resize
        if ('orientation' in window) {
            window.addEventListener('orientationchange', debouncedLayoutUpdate);
        } else {
             // Resize listener already added
        }

        // Initial message
        buttonGrid.innerHTML = 'Connecting to server...';
    }

    // --- Public API ---
    return {
        init,
        loadInitialData, // Exposed function to load data
        toggleFullScreen
    };
})();

window.uiModule = ui; 