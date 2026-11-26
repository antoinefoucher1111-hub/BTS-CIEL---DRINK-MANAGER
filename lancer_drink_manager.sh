#!/bin/bash
 
PROJET="/home/ciel/BTS-CIEL---DRINK-MANAGER/projet"
PORT_ARDUINO="/dev/ttyACM0"
PID_BACK=""
PID_WEB=""
 
cleanup() {
    echo ""
    echo "[INFO] Arrêt de tous les services Drink Manager..."
    [ -n "$PID_WEB" ] && kill "$PID_WEB" 2>/dev/null
    [ -n "$PID_BACK" ] && kill "$PID_BACK" 2>/dev/null
    exit 0
}
trap cleanup INT TERM
 
echo "[INFO] Nettoyage des anciens processus..."
pkill -x "drink_manager" 2>/dev/null || true
pkill -f "http\.server 8081" 2>/dev/null || true
pkill -f "raspberry_serial_bridge\.py" 2>/dev/null || true
sleep 1
 
echo "[1/3] Démarrage du backend C++ (port 8080)..."
cd "$PROJET/backend" || { echo "[ERREUR] Dossier backend introuvable"; exit 1; }
./build/drink_manager > /tmp/drink_backend.log 2>&1 &
PID_BACK=$!
sleep 2
 
if ! kill -0 "$PID_BACK" 2>/dev/null; then
    echo "[ERREUR] Le backend C++ n'a pas démarré. Log :"
    cat /tmp/drink_backend.log
    exit 1
fi
 
echo "[2/3] Démarrage de l'interface web (port 8081)..."
cd "$PROJET/drink-manager-web" || { echo "[ERREUR] Dossier web introuvable"; cleanup; }
python3 -m http.server 8081 --directory dist > /tmp/drink_web.log 2>&1 &
PID_WEB=$!
sleep 1
 
if ! kill -0 "$PID_WEB" 2>/dev/null; then
    echo "[ERREUR] Le serveur web n'a pas démarré. Log :"
    cat /tmp/drink_web.log
    cleanup
fi
 
IP_PI=$(hostname -I | awk '{print $1}')
echo "=============================================="
echo "  DRINK MANAGER PRÊT !"
echo "  Interface Web : http://${IP_PI}:8081"
echo "  API Backend   : http://${IP_PI}:8080/api/health"
echo "  (Appuyez sur Ctrl + C pour tout arrêter)"
echo "=============================================="
echo ""
 
echo "[3/3] Démarrage du pont série Arduino ($PORT_ARDUINO)..."
python3 "$PROJET/backend/scripts/raspberry_serial_bridge.py" --port "$PORT_ARDUINO"
cleanup
