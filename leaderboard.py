import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QShortcut, QInputDialog
from PyQt5.QtCore import QUrl, Qt
from PyQt5.QtGui import QKeySequence
from PyQt5.QtWebEngineWidgets import QWebEngineView


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Presensi Leaderboard RFID")

        self.browser = QWebEngineView()
        self.setCentralWidget(self.browser)

        # URL default
        self.default_url = "http://127.0.0.1:9000/presensi-edu/guest/leaderboard_usb_rfid/dcZrym"
        self.browser.load(QUrl(self.default_url))

        # Enable fitur JavaScript dan penyimpanan lokal
        self.browser.settings().setAttribute(self.browser.settings().JavascriptEnabled, True)
        self.browser.settings().setAttribute(self.browser.settings().JavascriptCanAccessClipboard, True)
        self.browser.settings().setAttribute(self.browser.settings().LocalStorageEnabled, True)

        # Fokus langsung agar bisa tangkap input dari RFID USB
        self.browser.setFocusPolicy(Qt.StrongFocus)
        self.browser.setFocus()

        # Shortcut Ctrl+X untuk keluar
        QShortcut(QKeySequence("Ctrl+X"), self, activated=self.close)

        # Shortcut Ctrl+E untuk edit URL
        QShortcut(QKeySequence("Ctrl+E"), self, activated=self.edit_url)

        # Shortcut Ctrl+R untuk reload halaman
        QShortcut(QKeySequence("Ctrl+R"), self, activated=self.browser.reload)

        # Full screen
        self.showFullScreen()

    def edit_url(self):
        new_url, ok = QInputDialog.getText(self, "Edit URL", "Masukkan URL baru:", text=self.browser.url().toString())
        if ok and new_url.strip():
            self.browser.load(QUrl(new_url.strip()))


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    sys.exit(app.exec_())
