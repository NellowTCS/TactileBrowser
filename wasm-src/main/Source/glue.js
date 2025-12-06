// WASM loader and interop (glue for modularized Emscripten output)

(function() {
  let ModuleInstance = null;

  function setStatus(msg, color) {
    const status = document.getElementById('status');
    if (!status) return;
    status.textContent = msg;
    if (color) status.style.color = color;
  }

  function resolveUrl(input) {
    try {
      if (/^https?:\/\//i.test(input)) {
        return input;
      }
      return new URL(input, window.location.href).toString();
    } catch (err) {
      console.warn('Failed to resolve URL, using raw value', err);
      return input;
    }
  }

  function buildProxyUrl(url) {
    if (!/^https?:\/\//i.test(url)) return null;
    return `https://r.jina.ai/${url}`;
  }

  function shouldRetryWithProxy(err) {
    if (!err) return false;
    const msg = String(err && err.message ? err.message : '').toLowerCase();
    return err.name === 'TypeError' || msg.includes('failed to fetch') || msg.includes('network error');
  }

  function fetchAndRenderInternal(targetUrl, displayUrl, options = {}) {
    const useProxy = Boolean(options.useProxy);
    const fetchOptions = { mode: targetUrl.startsWith('http') ? 'cors' : 'same-origin' };

    fetch(targetUrl, fetchOptions)
      .then((resp) => {
        if (!resp.ok) {
          throw new Error(`HTTP ${resp.status} ${resp.statusText}`);
        }
        return resp.text();
      })
      .then((html) => {
        ModuleInstance.ccall('render_html_from_js', 'void', ['string', 'string'], [displayUrl, html]);
      })
      .catch((err) => {
        if (!useProxy) {
          const proxyUrl = buildProxyUrl(displayUrl);
          if (proxyUrl && shouldRetryWithProxy(err)) {
            console.warn('Direct fetch failed, retrying via proxy', err);
            setStatus('Retrying via proxy...', '#999');
            fetchAndRenderInternal(proxyUrl, displayUrl, { useProxy: true });
            return;
          }
        }

        setStatus('Fetch failed', '#FF6B6B');
        console.error('Failed to fetch URL', err);
        try {
          const errMsg = err && err.message ? err.message : 'Unable to fetch content';
          ModuleInstance.ccall('wasm_display_message', 'void', ['string', 'number'], [errMsg, 0xFF6B6B]);
        } catch (_) {
          /* ignore */
        }
      });
  }

  // Public API used by C/C++ via EM_ASM and by the UI
  window.TactileBrowserWasm = {
    // Called from index.html after the modularized Emscripten module is instantiated.
    init: function(modInstance) {
      ModuleInstance = modInstance;

      try {
        window.Module = ModuleInstance;
      } catch (e) {
        // ignore
      }

      setStatus('Ready.', '#FFD93D');

      if (typeof ModuleInstance.onRuntimeInitialized === 'function') {
        const existing = ModuleInstance.onRuntimeInitialized;
        ModuleInstance.onRuntimeInitialized = function() {
          try { existing(); } catch (e) { console.error(e); }
          setStatus('Ready.', '#FFD93D');
        };
      }
    },

    fetchAndRender: function(rawUrl) {
      const input = (rawUrl || '').trim();
      if (!input) {
        setStatus('Enter a URL', '#FF6B6B');
        return;
      }

      if (!ModuleInstance || typeof ModuleInstance.ccall !== 'function') {
        setStatus('WASM not initialized', '#FF6B6B');
        console.error('Attempted to fetch before WASM module was ready');
        return;
      }

      const resolvedUrl = resolveUrl(input);
      setStatus('Loading...', '#999');
      fetchAndRenderInternal(resolvedUrl, resolvedUrl);
    },

    loadUrl: function(url) {
      this.fetchAndRender(url);
    },

    setStatus: function(msg, color) {
      setStatus(msg, color);
    }
  };
})();