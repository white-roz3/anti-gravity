FROM nginx:alpine

# nginx config: MIME types, asset caching, clean URLs.
# NOTE: the WASM build is single-threaded (no SharedArrayBuffer), so COOP/COEP
# headers are NOT needed — omitting them lets the landing page use web fonts.
RUN cat > /etc/nginx/conf.d/default.conf <<'EOF'
# Pick a Cache-Control value from the request path (query string is not part of $uri).
# Versioned assets (?v=agN) are safe to cache forever; HTML must revalidate.
map $uri $cache_control {
    default                   "no-cache, must-revalidate";
    "~*\.(?:js|wasm|data)$"   "public, max-age=31536000, immutable";
}

server {
    listen 8080;
    root /usr/share/nginx/html;
    index index.html;

    add_header Cache-Control $cache_control always;

    types {
        text/html                             html htm;
        application/javascript                js;
        application/wasm                      wasm;
        application/octet-stream              data;
    }

    # Clean URLs: /play -> play.html
    location / {
        try_files $uri $uri.html $uri/ =404;
    }
}
EOF

COPY build-wasm/index.html /usr/share/nginx/html/
COPY build-wasm/play.html /usr/share/nginx/html/
COPY build-wasm/antigravity.js /usr/share/nginx/html/
COPY build-wasm/antigravity.wasm /usr/share/nginx/html/

RUN apk add --no-cache curl && \
    for chunk in aa ab ac ad ae af ag ah ai aj ak al am an ao; do \
      curl -fL "https://github.com/white-roz3/anti-gravity/releases/download/v1.0/chunk_${chunk}" \
           -o "/tmp/chunk_${chunk}" || exit 1; \
    done && \
    cat /tmp/chunk_aa /tmp/chunk_ab /tmp/chunk_ac /tmp/chunk_ad /tmp/chunk_ae \
        /tmp/chunk_af /tmp/chunk_ag /tmp/chunk_ah /tmp/chunk_ai /tmp/chunk_aj \
        /tmp/chunk_ak /tmp/chunk_al /tmp/chunk_am /tmp/chunk_an /tmp/chunk_ao \
        > /usr/share/nginx/html/antigravity.data && \
    rm /tmp/chunk_a* && \
    apk del curl

EXPOSE 8080
