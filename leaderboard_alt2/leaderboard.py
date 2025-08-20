import sys
import subprocess
import ctypes
from shutil import which
from PyQt5.QtWidgets import QApplication, QMainWindow, QInputDialog, QShortcut
from PyQt5.QtGui import QKeySequence
from PyQt5.QtWebEngineWidgets import QWebEngineView
from PyQt5.QtCore import QUrl


class Controller(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Leaderboard QWebEngineView")

        self.base_url = "https://presensi.s-c-h.my.id/leaderboard/index/sMkMuH1m4"

        # Browser
        self.browser = QWebEngineView()
        self.browser.load(QUrl(self.base_url))
        self.setCentralWidget(self.browser)
        self.browser.setFocus()

        # Shortcuts
        QShortcut(QKeySequence("Ctrl+X"), self, activated=self.close)          # Keluar aplikasi
        QShortcut(QKeySequence("Ctrl+E"), self, activated=self.edit_url)       # Edit URL
        QShortcut(QKeySequence("Ctrl+R"), self, activated=self.browser.reload) # Reload
        QShortcut(QKeySequence("Esc"), self, activated=self.exit_fullscreen)   # Exit fullscreen
        QShortcut(QKeySequence("Ctrl+F"), self, activated=self.toggle_fullscreen) # Toggle fullscreen

        # Fullscreen awal
        self.showFullScreen()

        # Cegah sleep
        self.prevent_sleep()

    def prevent_sleep(self):
        """Cegah sleep/blank screen di macOS, Linux (desktop & Armbian), Windows"""
        if sys.platform == "darwin":  # macOS
            self.caffeinate = subprocess.Popen(["caffeinate", "-dimsu"])
            print("✅ Anti-sleep aktif (macOS caffeinate)")

        elif sys.platform.startswith("linux"):
            if which("xset"):  # Linux dengan X11
                subprocess.Popen(["xset", "s", "off"])
                subprocess.Popen(["xset", "-dpms"])
                subprocess.Popen(["xset", "s", "noblank"])
                print("✅ Anti-sleep aktif (Linux X11 via xset)")
            elif which("setterm"):  # Linux framebuffer (contoh: Armbian CLI)
                subprocess.Popen(
                    ["setterm", "-blank", "0", "-powerdown", "0"],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL
                )
                print("✅ Anti-sleep aktif (Linux framebuffer via setterm)")
            else:
                print("⚠️ Tidak ada xset atau setterm, anti-sleep tidak aktif.")

        elif sys.platform.startswith("win"):  # Windows
            ES_CONTINUOUS = 0x80000000
            ES_DISPLAY_REQUIRED = 0x00000002
            ES_SYSTEM_REQUIRED = 0x00000001
            ctypes.windll.kernel32.SetThreadExecutionState(
                ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED
            )
            print("✅ Anti-sleep aktif (Windows WinAPI)")

    def edit_url(self):
        new_url, ok = QInputDialog.getText(self, "Edit URL", "Masukkan URL baru:")
        if ok and new_url:
            self.base_url = new_url
            self.browser.load(QUrl(self.base_url))
            self.browser.setFocus()

    def exit_fullscreen(self):
        """Keluar dari fullscreen"""
        self.showNormal()

    def toggle_fullscreen(self):
        """Toggle fullscreen on/off"""
        if self.isFullScreen():
            self.showNormal()
        else:
            self.showFullScreen()

    def close(self):
        # Stop caffeinate kalau ada (macOS)
        if hasattr(self, "caffeinate"):
            self.caffeinate.terminate()
        QApplication.quit()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = Controller()
    sys.exit(app.exec_())
