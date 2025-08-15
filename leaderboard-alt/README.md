
pip install PyQt5
pip install PyQtWebEngine


chmod +x /home/username/leaderboard.py
Environment=DISPLAY=:0 diperlukan jika Anda pakai GUI (X11).


✅ 1. File systemd service
📄 Buat file ini: /etc/systemd/system/leaderboard.service
ini


[Unit]
Description=Presensi Leaderboard RFID (PyQt5)
After=network.target

[Service]
ExecStart=/usr/bin/python3 /home/armbian/leaderboard.py
WorkingDirectory=/home/armbian
StandardOutput=inherit
StandardError=inherit
Restart=always
User=armbian
Environment=DISPLAY=:0

[Install]
WantedBy=default.target

⚠️ Pastikan Anda ganti /home/armbian dan User=armbian dengan nama user dan lokasi file Anda jika berbeda.

🧩 Jalankan perintah berikut untuk mengaktifkan:


sudo systemctl daemon-reexec
sudo systemctl daemon-reload
sudo systemctl enable leaderboard.service
sudo systemctl start leaderboard.service
✅ 2. (Alternatif) Jika pakai GUI desktop, gunakan autostart .desktop
📄 Buat file ini: /home/armbian/.config/autostart/leaderboard.desktop
ini


[Desktop Entry]
Type=Application
Exec=python3 /home/armbian/leaderboard.py
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
Name=Leaderboard RFID
Jalur Python bisa dicek pakai: which python3 (biasanya /usr/bin/python3)

✅ Final Checklist
 Python file Anda berada di: /home/armbian/leaderboard.py

 Bisa dijalankan manual via terminal: python3 leaderboard.py

 Sudah menginstal: PyQt5, PyQtWebEngine

 Jika pakai GUI: environment DISPLAY=:0 harus tersedia

Jika sudah sesuai, reboot:


sudo reboot
Setelah boot, aplikasi akan langsung tampil fullscreen.

Kalau masih belum jalan setelah reboot, kirim isi log dengan:


journalctl -u leaderboard.service
Perlu bantuan auto-start juga untuk web server di port 9000?