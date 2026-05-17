# Deploy: monitoring.kreasikaryaarjuna.co.id

## Persyaratan
- aaPanel sudah terinstall PostgreSQL dan Python 3.11+
- Mosquitto MQTT broker (bisa share dengan echo_alert atau install baru)
- Subdomain `monitoring.kreasikaryaarjuna.co.id` sudah pointing ke IP server (sudah selesai di Cloudflare)

---

## Langkah Deploy

### 1. Buat database PostgreSQL
```bash
sudo -u postgres psql
CREATE USER dava WITH PASSWORD 'ganti_password_ini';
CREATE DATABASE dava_monitoring OWNER dava;
\q
```

### 2. Upload project ke server
```bash
rsync -avz --exclude '.git' --exclude '__pycache__' \
  ./ root@45.66.153.156:/www/wwwroot/monitoring.kreasikaryaarjuna.co.id/
```

### 3. Install Python dependencies
```bash
cd /www/wwwroot/monitoring.kreasikaryaarjuna.co.id/server
pip install -r requirements.txt
```

### 4. Buat .env
```bash
cp .env.example .env
nano .env
# Isi DATABASE_URL, MQTT_HOST, API_KEY
```

### 5. Setup nginx (temporary HTTP dulu untuk SSL challenge)
Buat vhost sementara di aaPanel untuk HTTP saja:
```bash
mkdir -p /www/wwwroot/monitoring.kreasikaryaarjuna.co.id/.well-known
```
Tambah site di aaPanel dengan domain `monitoring.kreasikaryaarjuna.co.id`, root ke folder tersebut.

### 6. Issue SSL dengan acme.sh
```bash
/root/.acme.sh/acme.sh --issue -d monitoring.kreasikaryaarjuna.co.id \
  --webroot /www/wwwroot/monitoring.kreasikaryaarjuna.co.id --keylength ec-256

/root/.acme.sh/acme.sh --install-cert -d monitoring.kreasikaryaarjuna.co.id --ecc \
  --cert-file     /www/server/panel/vhost/cert/monitoring.kreasikaryaarjuna.co.id/cert.pem \
  --key-file      /www/server/panel/vhost/cert/monitoring.kreasikaryaarjuna.co.id/privkey.pem \
  --fullchain-file /www/server/panel/vhost/cert/monitoring.kreasikaryaarjuna.co.id/fullchain.pem \
  --reloadcmd "/etc/init.d/nginx reload"
```

### 7. Pasang nginx config
```bash
mkdir -p /www/server/panel/vhost/cert/monitoring.kreasikaryaarjuna.co.id
cp /www/wwwroot/monitoring.kreasikaryaarjuna.co.id/deploy/nginx.conf \
   /www/server/panel/vhost/nginx/monitoring.kreasikaryaarjuna.co.id.conf
/etc/init.d/nginx reload
```

### 8. Jalankan dengan PM2
```bash
cd /www/wwwroot/monitoring.kreasikaryaarjuna.co.id
pm2 start server/ecosystem.config.js --only dava-monitoring-prod
pm2 save
```

---

## Update / Redeploy
```bash
# Upload ulang file
rsync -avz --exclude '.git' --exclude '__pycache__' --exclude '.env' \
  ./ root@45.66.153.156:/www/wwwroot/monitoring.kreasikaryaarjuna.co.id/

# Restart app
pm2 restart dava-monitoring-prod
```

## Cek logs
```bash
pm2 logs dava-monitoring-prod
tail -f /www/wwwlogs/dava-monitoring.error.log
```

## MQTT Broker
Jika server belum ada Mosquitto (cek: `systemctl status mosquitto`):
```bash
apt install mosquitto mosquitto-clients -y
systemctl enable mosquitto
systemctl start mosquitto
```
Jika echo_alert sudah pakai Mosquitto di port 1883, project ini bisa share broker yang sama — topicnya sudah beda (`dava/` vs `echo/`).
