import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import ttk
import threading
import time


class STM32DataViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("STM32 Veri İzleme Paneli - Sentinel-X")
        self.root.geometry("600x450")
        self.root.configure(bg="#2c3e50")

        self.serial_port = None
        self.is_running = False

        # UI Başlık hattı
        title_label = tk.Label(root, text="STM32 Hoca Laboratuvar Projesi Veri Alıcısı", font=("Helvetica", 16, "bold"),
                               fg="#ecf0f1", bg="#2c3e50")
        title_label.pack(pady=15)

        # Bağlantı Çerçevesi
        conn_frame = tk.Frame(root, bg="#2c3e50")
        conn_frame.pack(pady=10)

        tk.Label(conn_frame, text="Port Seçin:", font=("Helvetica", 10), fg="#ecf0f1", bg="#2c3e50").pack(side=tk.LEFT,
                                                                                                          padx=5)

        self.port_combobox = ttk.Combobox(conn_frame, values=[p.device for p in serial.tools.list_ports.comports()],
                                          width=15)
        self.port_combobox.pack(side=tk.LEFT, padx=5)
        if self.port_combobox.get() == "" and self.port_combobox['values']:
            self.port_combobox.current(0)

        self.btn_connect = tk.Button(conn_frame, text="Bağlan", command=self.toggle_connection, bg="#27ae60",
                                     fg="white", font=("Helvetica", 10, "bold"), width=13)
        self.btn_connect.pack(side=tk.LEFT, padx=10)

        # Veri Ekran Kartları
        display_frame = tk.Frame(root, bg="#2c3e50")
        display_frame.pack(pady=20, fill=tk.X, padx=40)

        # Sıcaklık Kartı
        self.temp_card = tk.Label(display_frame, text="Sıcaklık\n--.- °C", font=("Helvetica", 18, "bold"), fg="#e74c3c",
                                  bg="#34495e", width=14, height=3, relief=tk.RIDGE, bd=2)
        self.temp_card.grid(row=0, column=0, padx=10, pady=10)

        # Saat Kartı
        self.time_card = tk.Label(display_frame, text="Saat\n--:--:--", font=("Helvetica", 18, "bold"), fg="#f1c40f",
                                  bg="#34495e", width=14, height=3, relief=tk.RIDGE, bd=2)
        self.time_card.grid(row=0, column=1, padx=10, pady=10)

        # Tarih Kartı
        self.date_card = tk.Label(display_frame, text="Tarih\n--/--/----", font=("Helvetica", 14, "bold"), fg="#2ecc71",
                                  bg="#34495e", width=18, height=3, relief=tk.RIDGE, bd=2)
        self.date_card.grid(row=1, column=0, columnspan=2, padx=10, pady=10, sticky="ew")

        # Durum Çubuğu
        self.status_bar = tk.Label(root, text="Durum: Bağlantı Yok", bd=1, relief=tk.SUNKEN, anchor=tk.W, bg="#34495e",
                                   fg="#bdc3c7", font=("Helvetica", 9))
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    def toggle_connection(self):
        if not self.is_running:
            selected_port = self.port_combobox.get()
            if not selected_port:
                self.status_bar.config(text="Durum: Hata - Port Seçilmedi!")
                return
            try:
                self.serial_port = serial.Serial(selected_port, 115200, timeout=1)
                self.is_running = True
                self.btn_connect.config(text="Bağlantıyı Kes", bg="#c0392b")
                self.status_bar.config(text=f"Durum: {selected_port} Portuna Bağlanıldı.")

                # Arka planda verileri kaçırmadan okumak için Thread başlatıyoruz
                self.read_thread = threading.Thread(target=self.read_serial_data, daemon=True)
                self.read_thread.start()
            except Exception as e:
                self.status_bar.config(text=f"Durum: Bağlantı Hatası! {str(e)}")
        else:
            self.is_running = False
            if self.serial_port and self.serial_port.is_open:
                self.serial_port.close()
            self.btn_connect.config(text="Bağlan", bg="#27ae60")
            self.status_bar.config(text="Durum: Bağlantı Kapatıldı.")

    def read_serial_data(self):
        while self.is_running:
            try:
                if self.serial_port and self.serial_port.in_waiting > 0:
                    raw_line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()

                    # STM32'den gelen veri "DATA," belirteciyle başlıyor mu kontrolü
                    if raw_line.startswith("DATA,"):
                        parsed_tokens = raw_line.split(',')

                        # Başındaki DATA kelimesiyle birlikte toplam 4 token (parça) olmalı
                        if len(parsed_tokens) == 4:
                            _, temp_val, time_val, date_val = parsed_tokens  # 'DATA' metni çöpe, diğerleri değişkenlere

                            # UI Öğelerini Ana Thread üzerinde güncelleme güvenliği
                            self.root.after(0, self.update_gui_labels, temp_val, time_val, date_val)
            except Exception:
                break
            time.sleep(0.05)

    def update_gui_labels(self, temp, time_str, date_str):
        self.temp_card.config(text=f"Sıcaklık\n{temp} °C")
        self.time_card.config(text=f"Saat\n{time_str}")
        self.date_card.config(text=f"Tarih\n{date_str}")


if __name__ == "__main__":
    root = tk.Tk()
    app = STM32DataViewer(root)
    root.mainloop()