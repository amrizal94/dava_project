(function () {
  const dot = document.getElementById('sse-dot');
  let es;

  function connect() {
    es = new EventSource('/api/stream');

    es.onopen = () => {
      if (dot) { dot.classList.add('connected'); dot.title = 'SSE connected'; }
    };

    es.onmessage = (e) => {
      try {
        const ev = JSON.parse(e.data);
        (window.__sseHandlers || []).forEach(h => h(ev));
      } catch (_) {}
    };

    es.onerror = () => {
      if (dot) { dot.classList.remove('connected'); dot.title = 'SSE reconnecting...'; }
      es.close();
      setTimeout(connect, 3000);
    };
  }

  connect();
})();
