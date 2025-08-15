import webview
import sys
import threading
import time

# URL default
default_url = "https://presensi.s-c-h.my.id/leaderboard/index/sMkMuH1m4"
window = None

def on_load():
    """Fungsi yang dijalankan saat jendela dimuat."""
    print("Halaman web sudah dimuat. RFID reader bisa digunakan.")

def close_window():
    """Fungsi untuk menutup jendela."""
    if window:
        window.destroy()

def edit_url():
    """Fungsi untuk mengganti URL."""
    new_url = window.prompt("Masukkan URL baru:", "Edit URL", default_url)
    if new_url:
        window.load_url(new_url)

def listen_for_shortcuts():
    """Mendengarkan input dari keyboard untuk shortcut."""
    print("Shortcut aktif: Ctrl+E (edit URL), Ctrl+R (reload), Ctrl+X (keluar)")
    while True:
        try:
            # Note: Ini adalah implementasi placeholder.
            # `keyboard` atau pustaka lain diperlukan untuk mendeteksi shortcut secara global.
            # Alternatifnya, gunakan tombol GUI.
            time.sleep(1)
        except KeyboardInterrupt:
            break

def main():
    global window
    # Buat jendela web
    window = webview.create_window(
        "Presensi Leaderboard RFID",
        url=default_url,
        fullscreen=True
    )

    # Menambahkan event listener saat halaman dimuat
    window.events.loaded += on_load

    # Menambahkan event listener untuk menanggapi shortcut
    # webview tidak memiliki dukungan shortcut global built-in seperti PyQt.
    # Untuk kasus kiosk, ini biasanya dihandle di level sistem operasi
    # atau melalui input text di halaman web itu sendiri.
    
    # Jalankan aplikasi. Blocking call.
    webview.start(private_mode=False) # `private_mode=False` mengaktifkan cache dan penyimpanan lokal

if __name__ == '__main__':
    main()
