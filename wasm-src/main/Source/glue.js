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

  const DEFAULT_PROXY_CHAIN = [
    {
      label: 'WasmWeb Backend',
      template: 'https://wasmweb-backend.neeljaiswal23.workers.dev/?url={{URL}}'
    }
  ];

  function getProxyConfigOverride() {
    const globalObj = typeof window !== 'undefined' ? window : (typeof self !== 'undefined' ? self : null);
    if (!globalObj) return null;
    const chain = globalObj.TactileBrowserWasmProxyChain;
    if (!chain || !Array.isArray(chain)) return null;
    return chain;
  }

  function normalizeProxyEntry(entry, index) {
    if (!entry) return null;
    if (typeof entry === 'string') {
      return { template: entry, encode: true, label: `proxy-${index + 1}` };
    }
    if (typeof entry === 'function') {
      return { builder: entry, label: entry.name || `proxy-${index + 1}` };
    }
    if (typeof entry === 'object') {
      if (entry.template || entry.prefix || entry.builder) {
        return {
          template: entry.template || entry.prefix || null,
          encode: entry.encode !== false,
          label: entry.label || `proxy-${index + 1}`,
          builder: entry.builder || null,
          fetchOptions: entry.fetchOptions || null
        };
      }
    }
    return null;
  }

  function buildProxyTarget(entry, url) {
    if (!entry) return null;
    if (entry.builder) {
      try {
        return entry.builder(url);
      } catch (err) {
        console.warn('Custom proxy builder threw', err);
        return null;
      }
    }

    if (!entry.template) return null;
    const encode = entry.encode !== false;
    const tokenValue = encode ? encodeURIComponent(url) : url;
    if (entry.template.includes('{{URL}}')) {
      return entry.template.replace(/\{\{URL\}\}/g, tokenValue);
    }
    return `${entry.template}${tokenValue}`;
  }

  function buildFetchChain(displayUrl) {
    const chain = [];
    const directOptions = { mode: displayUrl.startsWith('http') ? 'cors' : 'same-origin' };
    chain.push({
      label: 'direct',
      target: displayUrl,
      options: directOptions
    });

    const override = getProxyConfigOverride();
    const sources = override && override.length ? override : DEFAULT_PROXY_CHAIN;

    sources.forEach((entry, idx) => {
      const normalized = normalizeProxyEntry(entry, idx);
      if (!normalized) return;
      const target = buildProxyTarget(normalized, displayUrl);
      if (!target) return;
      chain.push({
        label: normalized.label || `proxy-${idx + 1}`,
        target,
        options: Object.assign({ mode: 'cors' }, normalized.fetchOptions || {})
      });
    });

    return chain;
  }

  function fetchViaChain(displayUrl, chain, index) {
    if (index >= chain.length) {
      setStatus('Fetch failed', '#FF6B6B');
      const errMsg = 'Unable to fetch content';
      try {
        ModuleInstance.ccall('wasm_display_message', 'void', ['string', 'number'], [errMsg, 0xFF6B6B]);
      } catch (_) {
        /* ignore */
      }
      return;
    }

    const candidate = chain[index];
    fetch(candidate.target, candidate.options)
      .then((resp) => {
        if (!resp.ok) {
          throw new Error(`HTTP ${resp.status} ${resp.statusText}`);
        }
        return resp.text();
      })
      .then((html) => {
        ModuleInstance.ccall('wasm_render_html', 'void', ['string', 'string'], [displayUrl, html]);
      })
      .catch((err) => {
        console.warn(`Fetch attempt via ${candidate.label || 'candidate'} failed`, err);
        if (index + 1 < chain.length) {
          const nextLabel = chain[index + 1].label || 'proxy';
          setStatus(`Retrying via ${nextLabel}...`, '#999');
          fetchViaChain(displayUrl, chain, index + 1);
          return;
        }

        setStatus('Fetch failed', '#FF6B6B');
        try {
          const errMsg = err && err.message ? err.message : 'Unable to fetch content';
          ModuleInstance.ccall('wasm_display_message', 'void', ['string', 'number'], [errMsg, 0xFF6B6B]);
        } catch (_) {
          /* ignore */
        }
      });
  }

  function fetchWithFallbacks(displayUrl) {
    const chain = buildFetchChain(displayUrl);
    fetchViaChain(displayUrl, chain, 0);
  }

  window.TactileBrowserWasm = {
    init: function(modInstance) {
      ModuleInstance = modInstance;
      try {
        window.Module = ModuleInstance;
      } catch (e) {
        // ignore
      }

      if (typeof ModuleInstance.ccall === 'function') {
        try {
          ModuleInstance.ccall('start_app', 'void', [], []);
        } catch (e) {
          console.error('Failed to call start_app:', e);
        }
      }

      setStatus('Ready.', '#FFD93D');
    },

    fetchAndRender: function(rawUrl) {
      const input = (rawUrl || '').trim();
      if (!input) {
        setStatus('Enter a URL', '#FF6B6B');
        return;
      }

      if (!ModuleInstance || typeof ModuleInstance.ccall !== 'function') {
        setStatus('WASM not initialized', '#FF6B6B');
        return;
      }

      const resolvedUrl = resolveUrl(input);
      setStatus('Loading...', '#999');
      fetchWithFallbacks(resolvedUrl);
    },

    loadUrl: function(url) {
      this.fetchAndRender(url);
    },

    setStatus: function(msg, color) {
      setStatus(msg, color);
    }
  };
})();
