// web/js/ui.js

const ui = (() => {
    // Cache DOM elements
    const buttonGrid = document.getElementById('button-grid');
    const appTitle = document.getElementById('app-title');
    const paginationDotsContainer = document.getElementById('pagination-dots');
    const connectionStatusDiv = document.getElementById('connection-status');

    let currentPageIndex = 0;
    let currentButtonLayout = null; // Store the layout data

    // Touch swipe state variables
    let touchStartX = 0;
    let touchMoveX = 0;
    let isSwiping = false;
    const swipeThreshold = 50; // Minimum pixels to be considered a swipe

    // --- Button Loading Logic --- 
    function loadButtons(buttonLayout = null) {
        // Store the layout data if provided, otherwise use existing stored data
        if (buttonLayout !== null) {
             currentButtonLayout = buttonLayout; 
        }

        // --- Reset State --- 
        buttonGrid.innerHTML = ''; 
        paginationDotsContainer.innerHTML = '';
        paginationDotsContainer.style.display = 'none'; 
        currentPageIndex = 0; 
        buttonGrid.className = ''; // Reset classes
        buttonGrid.style.cssText = ''; // Reset inline styles
        // Remove potential old listeners before deciding layout
        removeSwipeListeners(); 

        // Check if we have layout data to work with
        if (!currentButtonLayout || currentButtonLayout.length === 0) {
            console.log("No button layout available to load.");
            buttonGrid.innerHTML = 'No buttons configured.'; // Clear grid and show message
            return;
        }

        const isPortrait = window.matchMedia("(orientation: portrait)").matches;
        // Use the stored layout data
        const needsPagination = currentButtonLayout.length > config.buttonsPerPage && !isPortrait;

        console.log(`Reloading layout - isPortrait: ${isPortrait}, needsPagination: ${needsPagination}`);

        if (needsPagination) {
            loadButtonsPaged(currentButtonLayout); 
            addSwipeListeners(); // Add swipe listeners only for paged layout
        } else {
            loadButtonsSimpleGrid(currentButtonLayout, isPortrait);
             // Ensure listeners removed if switching from paged
             removeSwipeListeners();
        }
    }

    // --- Simple Grid Loading --- 
    function loadButtonsSimpleGrid(buttonLayout, isPortrait) {
        console.log(`Loading simple grid (Portrait: ${isPortrait})`);
        buttonGrid.classList.remove('paginated');
        buttonGrid.classList.toggle('landscape-simple-grid', !isPortrait);
        
        buttonLayout.forEach(button => {
            if (!button.id || !button.name) return; 
            const btnElement = createButtonElement(button); 
            btnElement.style.aspectRatio = ''; 
            buttonGrid.appendChild(btnElement);
        });
    }

    // --- Paged Grid Loading --- 
    function loadButtonsPaged(buttonLayout) {
        console.log("Loading paged layout.");
        buttonGrid.classList.add('paginated');
        buttonGrid.classList.remove('landscape-simple-grid'); 
        
        const totalPages = Math.ceil(buttonLayout.length / config.buttonsPerPage);
        const pagesContainer = document.createElement('div');
        pagesContainer.className = 'button-pages-container';
        buttonGrid.appendChild(pagesContainer);

        for (let i = 0; i < totalPages; i++) {
            const pageElement = document.createElement('div');
            pageElement.className = 'button-page'; 
            pagesContainer.appendChild(pageElement);
            pageElement.id = `button-page-${i}`;

            const pageButtons = buttonLayout.slice(i * config.buttonsPerPage, (i + 1) * config.buttonsPerPage);
            pageButtons.forEach(button => {
                if (!button.id || !button.name) return; 
                const btnElement = createButtonElement(button); 
                btnElement.style.aspectRatio = '1 / 1'; 
                pageElement.appendChild(btnElement);
            });
        }
        
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
        } 
        goToPage(0);
    }

    // --- Helper: Create Button Element --- 
    function createButtonElement(button) {
        const btnElement = document.createElement('button');
        btnElement.className = 'grid-button';
        btnElement.onclick = () => window.websocketService.sendButtonPress(button.id);

        if (button.icon_path && button.icon_path.trim() !== '') {
            btnElement.classList.add('has-icon');
            const imgElement = document.createElement('img');
            imgElement.src = button.icon_path; 
            imgElement.alt = button.name;
            imgElement.onerror = () => {
                console.error(`Failed to load icon: ${button.icon_path}`);
                imgElement.remove();
                if (!btnElement.querySelector('.button-text')) {
                     const textElementFallback = document.createElement('span');
                     textElementFallback.className = 'button-text';
                     textElementFallback.textContent = button.name;
                     btnElement.appendChild(textElementFallback);
                }
            };
            btnElement.appendChild(imgElement);

            const textElement = document.createElement('span');
            textElement.className = 'button-text';
            textElement.textContent = button.name;
            btnElement.appendChild(textElement);
        } else {
            btnElement.textContent = button.name;
        }
        return btnElement;
    }

    // --- Touch Swipe Handling --- 
    function addSwipeListeners() {
        console.log("Adding swipe listeners");
        buttonGrid.addEventListener('touchstart', handleTouchStart, { passive: true }); // Use passive for start
        buttonGrid.addEventListener('touchmove', handleTouchMove, { passive: false }); // Need false to preventDefault
        buttonGrid.addEventListener('touchend', handleTouchEnd, { passive: true });
    }

    function removeSwipeListeners() {
        console.log("Removing swipe listeners");
        buttonGrid.removeEventListener('touchstart', handleTouchStart);
        buttonGrid.removeEventListener('touchmove', handleTouchMove);
        buttonGrid.removeEventListener('touchend', handleTouchEnd);
    }

    function handleTouchStart(event) {
        // Ignore multi-touch or if not in paginated mode
        if (event.touches.length > 1 || !buttonGrid.classList.contains('paginated')) return;
        touchStartX = event.touches[0].clientX;
        touchMoveX = touchStartX; // Initialize moveX
        isSwiping = false; // Reset swipe flag
        // console.log('Touch Start:', touchStartX);
    }

    function handleTouchMove(event) {
         // Ignore multi-touch or if not in paginated mode
        if (event.touches.length > 1 || !buttonGrid.classList.contains('paginated')) return;
       
        touchMoveX = event.touches[0].clientX;
        const deltaX = touchMoveX - touchStartX;
        
        // If significant horizontal movement, start considering it a swipe
        // and prevent vertical scrolling.
        if (Math.abs(deltaX) > 10 && !isSwiping) { 
             console.log("Swipe detected, preventing vertical scroll");
             isSwiping = true;
        }
        
        if (isSwiping) {
            // Prevent default only when actively swiping horizontally
            event.preventDefault(); 
        }
        // console.log('Touch Move:', touchMoveX);
    }

    function handleTouchEnd(event) {
        // console.log('Touch End:', touchMoveX);
         // Ignore if not in paginated mode or wasn't a swipe
        if (!buttonGrid.classList.contains('paginated') || !isSwiping) { 
            resetSwipeState();
            return;
        }

        const deltaX = touchMoveX - touchStartX;
        const totalPages = buttonGrid.querySelectorAll('.button-page').length;

        // console.log('DeltaX:', deltaX);

        if (deltaX < -swipeThreshold) {
            // Swipe Left (Next Page)
            console.log("Swipe Left occurred");
            goToPage(Math.min(currentPageIndex + 1, totalPages - 1));
        } else if (deltaX > swipeThreshold) {
            // Swipe Right (Previous Page)
            console.log("Swipe Right occurred");
            goToPage(Math.max(currentPageIndex - 1, 0));
        } else {
             console.log("Swipe distance below threshold");
        }

        resetSwipeState();
    }
    
    function resetSwipeState() {
        touchStartX = 0;
        touchMoveX = 0;
        isSwiping = false;
    }

    // --- Pagination Navigation (Show/Hide Logic) --- 
    function goToPage(pageIndex) {
        const pages = buttonGrid.querySelectorAll('.button-page'); 
        if (pages.length === 0) return;

        const totalPages = pages.length;
        if (pageIndex < 0 || pageIndex >= totalPages) {
            console.warn(`Invalid page index: ${pageIndex}`);
            pageIndex = Math.max(0, Math.min(pageIndex, totalPages - 1));
        }

        // Check if page actually changed before toggling classes
        if(pageIndex === currentPageIndex && pages[pageIndex]?.classList.contains('active')) {
            // console.log("Already on target page:", pageIndex);
            return; // Avoid unnecessary DOM manipulation if already on the target page
        }

        console.log(`Going to page: ${pageIndex}`);
        currentPageIndex = pageIndex;

        pages.forEach((page, index) => {
            page.classList.toggle('active', index === currentPageIndex);
        });
        
        updatePaginationDots();
    }

    // --- Update Pagination Dots --- 
    function updatePaginationDots() {
        const dots = paginationDotsContainer.querySelectorAll('.dot');
        dots.forEach((dot, index) => {
            dot.classList.toggle('active', index === currentPageIndex);
        });
        paginationDotsContainer.style.display = dots.length > 1 ? 'flex' : 'none'; 
    }

    // --- Fullscreen --- 
    function toggleFullScreen() {
        if (!document.fullscreenElement) {
            if (document.documentElement.requestFullscreen) {
                document.documentElement.requestFullscreen();
            } else if (document.documentElement.webkitRequestFullscreen) { /* Safari */
                document.documentElement.webkitRequestFullscreen();
            } else if (document.documentElement.msRequestFullscreen) { /* IE11 */
                document.documentElement.msRequestFullscreen();
            }
        } else {
            if (document.exitFullscreen) {
                document.exitFullscreen();
            } else if (document.webkitExitFullscreen) { /* Safari */
                document.webkitExitFullscreen();
            } else if (document.msExitFullscreen) { /* IE11 */
                document.msExitFullscreen();
            }
        }
    }

    // --- Initialization --- 
    function init() {
        if (appTitle) {
            appTitle.addEventListener('click', toggleFullScreen);
        }
        // WebSocket connection initiated from main.js
    }

    // --- Public API --- 
    return {
        init,
        loadButtons,
        connectionStatusDiv
    };

})();

window.uiModule = ui; 