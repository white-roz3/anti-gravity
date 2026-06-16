FROM nginx:alpine

# nginx config with correct MIME types and COOP/COEP headers for SharedArrayBuffer
RUN cat > /etc/nginx/conf.d/default.conf <<'EOF'
server {
    listen 8080;
    root /usr/share/nginx/html;
    index index.html;

    add_header Cross-Origin-Opener-Policy "same-origin";
    add_header Cross-Origin-Embedder-Policy "require-corp";

    types {
        text/html                             html htm;
        application/javascript                js;
        application/wasm                      wasm;
        application/octet-stream              data;
    }

    location / {
        try_files $uri $uri/ =404;
    }
}
EOF

COPY build-wasm/index.html /usr/share/nginx/html/
COPY build-wasm/game.html /usr/share/nginx/html/
COPY build-wasm/antigravity.js /usr/share/nginx/html/
COPY build-wasm/antigravity.wasm /usr/share/nginx/html/
COPY build-wasm/antigravity.data /usr/share/nginx/html/

EXPOSE 8080
