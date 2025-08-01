import sys
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtCore import QUrl, Qt
from PyQt5.QtWebEngineWidgets import QWebEngineView


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Presensi Leaderboard RFID")

        self.browser = QWebEngineView()
        self.setCentralWidget(self.browser)

        # Load halaman leaderboard
        self.browser.load(QUrl("http://127.0.0.1:9000/presensi-edu/guest/leaderboard_usb_rfid/dcZrym"))

        # Enable fitur JavaScript dan penyimpanan lokal
        self.browser.settings().setAttribute(self.browser.settings().JavascriptEnabled, True)
        self.browser.settings().setAttribute(self.browser.settings().JavascriptCanAccessClipboard, True)
        self.browser.settings().setAttribute(self.browser.settings().LocalStorageEnabled, True)

        # Fokus langsung agar bisa tangkap input dari RFID USB
        self.browser.setFocusPolicy(Qt.StrongFocus)
        self.browser.setFocus()

        # Full screen
        self.showFullScreen()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    sys.exit(app.exec_())


