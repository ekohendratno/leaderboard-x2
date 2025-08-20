import sys
import os
import subprocess
import ctypes
from shutil import which
from PyQt5.QtWidgets import QApplication, QMainWindow, QInputDialog, QShortcut
from PyQt5.QtGui import QKeySequence
from PyQt5.QtCore import Qt


class ExternalChromiumController(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Leaderboard External Chromium")

        # ==== Konfigurasi dasar ====
        self.base_url = "https://presensi.s-c-h.my.id/leaderboard/index/sMkMuH1m4"
        self.fullscreen = True
        self.chromium = None
        self.caffeinate = None

        # ==== Temukan executable browser ====
        self.browser_cmd = self.find_chromium()
        if not self.browser_cmd:
            raise RuntimeError("❌ Tidak menemukan Chromium/Chrome/Edge di sistem!")

        # ==== Tampilkan window mungil agar shortcut aktif ====
        # (hampir tak terlihat, tidak mengganggu tampilan kiosk)
        self.setWindowFlags(Qt.Tool)           # tidak muncul di taskbar/dock
        self.setGeometry(50, 50, 2, 2)         # mungil
        self.setWindowOpacity(0.04)            # hampir transparan
        self.show()                             # penting: agar QShortcut aktif
        self.activateWindow()

        # ==== Shortcuts ====
        QShortcut(QKeySequence("Ctrl+X"), self, activated=self.close_all)        # keluar app
        QShortcut(QKeySequence("Ctrl+E"), self, activated=self.edit_url)         # ganti URL
        QShortcut(QKeySequence("Ctrl+R"), self, activated=self.reload_browser)   # reload
        QShortcut(QKeySequence("Esc"),    self, activated=self.exit_fullscreen)  # keluar fullscreen
        QShortcut(QKeySequence("Ctrl+F"), self, activated=self.toggle_fullscreen)# toggle fullscreen

        # ==== Anti-sleep ====
        self.prevent_sleep()

        # ==== Jalankan browser pertama kali ====
        self.launch_browser(fullscreen=True)

    # ------------------------------ Browser helpers ------------------------------

    def find_chromium(self):
        """Cari path browser eksternal sesuai platform"""
        cand = []
        if sys.platform == "darwin":  # macOS
            cand = [
                "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
                "/Applications/Chromium.app/Contents/MacOS/Chromium",
                "/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge",
            ]
        elif sys.platform.startswith("linux"):
            cand = ["chromium", "chromium-browser", "google-chrome", "google-chrome-stable", "microsoft-edge"]
        elif sys.platform.startswith("win"):
            cand = [
                os.path.expandvars(r"%ProgramFiles%\Google\Chrome\Application\chrome.exe"),
                os.path.expandvars(r"%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe"),
                os.path.expandvars(r"%ProgramFiles%\Microsoft\Edge\Application\msedge.exe"),
                os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft\Edge\Application\msedge.exe"),
            ]

        for c in cand:
            if not c:
                continue
            if which(c) or os.path.exists(c):
                return c
        return None

    def _build_args(self, fullscreen: bool):
        """Bangun argumen Chromium/Chrome/Edge yang konsisten di semua OS."""
        args = [
            self.browser_cmd,
            "--no-first-run",
            "--disable-logging",
            "--noerrdialogs",
            "--disable-infobars",
            "--disable-session-crashed-bubble",
            "--overscroll-history-navigation=0",
            "--incognito",  # selalu incognito
            "--password-store=basic",
        ]

        # Wayland support (jika variabel env terdeteksi)
        if "WAYLAND_DISPLAY" in os.environ:
            args += ["--enable-features=UseOzonePlatform", "--ozone-platform=wayland"]

        # Kiosk/fullscreen
        if fullscreen:
            # kombinasi ini paling konsisten untuk fullscreen
            args += ["--kiosk", "--start-fullscreen"]

        # URL taruh paling akhir
        args.append(self.base_url)
        return args

    def launch_browser(self, fullscreen=True):
        """Jalankan/ulang browser eksternal sesuai mode."""
        self.fullscreen = fullscreen
        # Matikan instance lama (jika ada)
        if self.chromium and self.chromium.poll() is None:
            self.chromium.terminate()
            try:
                self.chromium.wait(timeout=3)
            except Exception:
                self.chromium.kill()

        args = self._build_args(fullscreen=fullscreen)
        self.chromium = subprocess.Popen(args)

    def reload_browser(self):
        """Reload (relaunch) browser dengan mode saat ini."""
        self.launch_browser(fullscreen=self.fullscreen)

    def edit_url(self):
        """Edit URL lalu reload."""
        new_url, ok = QInputDialog.getText(self, "Edit URL", "Masukkan URL baru:")
        if ok and new_url:
            self.base_url = new_url.strip()
            self.reload_browser()

    def exit_fullscreen(self):
        """Keluar dari fullscreen (relaunch tanpa --kiosk)."""
        if self.fullscreen:
            self.launch_browser(fullscreen=False)

    def toggle_fullscreen(self):
        """Toggle fullscreen ↔ normal (relaunch)."""
        self.launch_browser(fullscreen=not self.fullscreen)

    # ------------------------------ Anti sleep ------------------------------

    def prevent_sleep(self):
        """Cegah sleep/blank cross-platform (macOS, Linux/Armbian, Windows)"""
        try:
            if sys.platform == "darwin":  # macOS
                # -d: disk, -i: system idle, -m: display, -s: system
                self.caffeinate = subprocess.Popen(["caffeinate", "-dimsu"])
                print("✅ Anti-sleep aktif (macOS caffeinate)")
            elif sys.platform.startswith("linux"):
                if which("xset"):
                    subprocess.Popen(["xset", "s", "off"])
                    subprocess.Popen(["xset", "-dpms"])
                    subprocess.Popen(["xset", "s", "noblank"])
                    print("✅ Anti-sleep aktif (Linux X11 via xset)")
                elif which("setterm"):
                    subprocess.Popen(
                        ["setterm", "-blank", "0", "-powerdown", "0"],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
                    )
                    print("✅ Anti-sleep aktif (Linux framebuffer via setterm)")
                else:
                    print("⚠️ Tidak ada xset/setterm, anti-sleep tidak aktif.")
            elif sys.platform.startswith("win"):
                ES_CONTINUOUS = 0x80000000
                ES_DISPLAY_REQUIRED = 0x00000002
                ES_SYSTEM_REQUIRED = 0x00000001
                ctypes.windll.kernel32.SetThreadExecutionState(
                    ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED
                )
                print("✅ Anti-sleep aktif (Windows WinAPI)")
        except Exception as e:
            print(f"⚠️ Gagal mengaktifkan anti-sleep: {e}")

    # ------------------------------ Cleanup ------------------------------

    def close_all(self):
        """Tutup browser & aplikasi (Ctrl+X)."""
        self._cleanup_and_quit()

    def closeEvent(self, event):
        """Pastikan proses anak dibereskan saat window ditutup."""
        self._cleanup_and_quit()
        event.accept()

    def _cleanup_and_quit(self):
        if self.chromium and self.chromium.poll() is None:
            self.chromium.terminate()
            try:
                self.chromium.wait(timeout=2)
            except Exception:
                self.chromium.kill()
        if self.caffeinate and self.caffeinate.poll() is None:
            self.caffeinate.terminate()
        QApplication.quit()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = ExternalChromiumController()
    sys.exit(app.exec_())
