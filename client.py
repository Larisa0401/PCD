import tkinter as tk
from tkinter import filedialog, messagebox
from PIL import Image, ImageTk
import socket
import struct
import os

class ImageEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Image Editor")
        self.root.geometry("800x800")

        self.original_image = None
        self.modified_image = None
        self.image_path = ""
        self.server_socket = None

        self.create_widgets()

    def create_widgets(self):
        self.image_label = tk.Label(self.root)
        self.image_label.pack()

        self.btn_load = tk.Button(self.root, text="Load Image", command=self.load_image, width=20)
        self.btn_load.pack()

        self.btn_connect_server = tk.Button(self.root, text="Connect to Server", command=self.connect_to_server, width=20)
        self.btn_connect_server.pack()

        self.btn_grayscale = tk.Button(self.root, text="Grayscale", command=self.send_grayscale, width=20)
        self.btn_grayscale.pack()

        self.btn_invert = tk.Button(self.root, text="Invert", command=self.send_invert, width=20)
        self.btn_invert.pack()

        self.btn_brightness = tk.Button(self.root, text="Brightness", command=self.send_brightness, width=20)
        self.btn_brightness.pack()

        self.btn_reset = tk.Button(self.root, text="Reset", command=self.reset_image, width=20)
        self.btn_reset.pack()

        self.btn_save = tk.Button(self.root, text="Save Image", command=self.save_image, width=20)
        self.btn_save.pack()

        self.btn_view_modified = tk.Button(self.root, text="View Modified Image", command=self.view_modified_image, width=20)
        self.btn_view_modified.pack()

        self.btn_sepia = tk.Button(self.root, text="Sepia", command=self.send_sepia, width=20)
        self.btn_sepia.pack()

        self.btn_gamma = tk.Button(self.root, text="Gamma", command=self.send_gamma, width=20)
        self.btn_gamma.pack()

        self.btn_crop = tk.Button(self.root, text="Crop", command=self.send_crop, width=20)
        self.btn_crop.pack()

        self.btn_sharpening = tk.Button(self.root, text="Sharpening", command=self.send_sharpening, width=20)
        self.btn_sharpening.pack()

        self.btn_blur = tk.Button(self.root, text="Blur", command=self.send_blur, width=20)
        self.btn_blur.pack()

        self.btn_edge_detection = tk.Button(self.root, text="Edge Detection", command=self.send_edge_detection, width=20)
        self.btn_edge_detection.pack()

        self.btn_contrast = tk.Button(self.root, text="Contrast", command=self.send_contrast, width=20)
        self.btn_contrast.pack()

        self.btn_rotate = tk.Button(self.root, text="Rotate", command=self.send_rotate, width=20)
        self.btn_rotate.pack()

        self.btn_disconnect_server = tk.Button(self.root, text="Disconnect from Server", command=self.disconnect_from_server, width=20)
        self.btn_disconnect_server.pack()

    def load_image(self):
        self.image_path = filedialog.askopenfilename(filetypes=[("Image Files", "*.jpg;*.jpeg;*.png")])
        if self.image_path:
            self.original_image = Image.open(self.image_path)
            self.modified_image = self.original_image.copy()
            self.display_image(self.modified_image)

    def display_image(self, image):
        img_tk = ImageTk.PhotoImage(image.resize((400, 400), Image.ANTIALIAS))
        self.image_label.config(image=img_tk)
        self.image_label.image = img_tk

    def connect_to_server(self):
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.connect(('localhost', 12349))
            messagebox.showinfo("Info", "Connected to server")
            self.btn_connect_server.pack_forget()
        except Exception as e:
            messagebox.showerror("Error", f"Failed to connect to server: {e}")

    def disconnect_from_server(self):
        if self.server_socket:
            try:
                self.server_socket.close()
                messagebox.showinfo("Info", "Disconnected from server")
                self.root.quit()
            except Exception as e:
                messagebox.showerror("Error", f"Failed to disconnect from server: {e}")

    def send_to_server(self, command):
        if not self.image_path:
            messagebox.showerror("Error", "No image loaded")
            return

        try:
            # Trimite comanda la server
            self.server_socket.sendall(command.encode())
            print(f"Command sent: {command}")

            # Deschide imaginea pentru citire binară
            with open(self.image_path, 'rb') as f:
                image_data = f.read()

            image_size = len(image_data)
            self.server_socket.sendall(struct.pack('!I', image_size))
            self.server_socket.sendall(image_data)
            print(f"Image sent: {self.image_path} (size: {image_size} bytes)")

            received_size = struct.unpack('!I', self.server_socket.recv(4))[0]
            received_data = b''
            while len(received_data) < received_size:
                packet = self.server_socket.recv(4096)
                if not packet:
                    break
                received_data += packet

            with open("modified_image.jpg", 'wb') as f:
                f.write(received_data)

            self.modified_image = Image.open("modified_image.jpg")
            self.display_image(self.modified_image)
        except Exception as e:
            messagebox.showerror("Error", f"Failed to communicate with server: {e}")

    def send_grayscale(self):
        self.send_to_server("grayscale")

    def send_invert(self):
        self.send_to_server("invert")

    def send_brightness(self):
        self.send_to_server("brightness")

    def send_sepia(self):
        self.send_to_server("sepia")

    def send_gamma(self):
        self.send_to_server("gamma")

    def send_crop(self):
        self.send_to_server("crop")

    def send_sharpening(self):
        self.send_to_server("sharpening")

    def send_blur(self):
        self.send_to_server("blur")

    def send_edge_detection(self):
        self.send_to_server("edge_detection")

    def send_contrast(self):
        self.send_to_server("contrast")

    def send_rotate(self):
        self.send_to_server("rotate")

    def save_image(self):
        if not self.modified_image:
            messagebox.showerror("Error", "No image to save")
            return

        save_path = filedialog.asksaveasfilename(defaultextension=".jpg")
        if save_path:
            self.modified_image.save(save_path)

    def view_modified_image(self):
        if self.modified_image:
            self.display_image(self.modified_image)
        else:
            messagebox.showerror("Error", "No modified image to display")

    def reset_image(self):
        if self.original_image:
            self.modified_image = self.original_image.copy()
            self.display_image(self.modified_image)

if __name__ == "__main__":
    root = tk.Tk()
    app = ImageEditor(root)
    root.mainloop()
