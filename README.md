# Firmware Máy Phủ Quay (Spin Coater)

Đây là firmware điều khiển máy phủ quay (spin coater) sử dụng vi điều khiển STM32F103C8. Hệ thống điều khiển tốc độ quay theo nhiều chế độ khác nhau và giao tiếp với ứng dụng máy tính để người dùng cấu hình các tham số phù hợp với mẫu vật liệu cần phủ.

## Tổng quan kiến trúc

Firmware được tổ chức theo kiến trúc phân lớp, mỗi lớp đảm nhận một chức năng cụ thể:

- **Drivers ([`firmware/drivers/`](firmware/drivers/))**: Lớp trừu tượng hóa các ngoại vi cơ bản (GPIO, UART, Timer, DMA) và cung cấp các hàm nguyên thủy như delay, truyền nhận UART.
- **Components ([`firmware/components/`](firmware/components/))**: Các module điều khiển phần cứng cụ thể như đọc encoder, điều khiển ESC, quản lý IO phụ trợ.
- **Control Algorithms ([`firmware/control_algorithms/`](firmware/control_algorithms/))**: Thư viện thuật toán điều khiển PID và bộ lọc Kalman để ổn định tốc độ quay.
- **Modes ([`firmware/modes/`](firmware/modes/))**: Định nghĩa các chế độ phủ (analog, digital, ramp, coater) dựa trên các thuật toán điều khiển.
- **Protocol UART ([`firmware/protocol_UART/`](firmware/protocol_UART/))**: Xử lý chuyển đổi và đóng gói khung dữ liệu để giao tiếp với ứng dụng host.
- **Host Apps ([`host_apps/`](host_apps/))**: Ứng dụng điều khiển và giao diện người dùng (tài liệu này tập trung vào firmware, nhưng thư mục này được cung cấp để tham khảo).

## Luồng điều khiển tốc độ

1. Người dùng chọn chế độ phủ và các tham số thông qua ứng dụng host hoặc màn hình LVGL tích hợp.
2. Các tham số được gửi đến MCU thông qua giao thức UART hoặc được cấu hình cố định trong chế độ hoạt động.
3. Encoder phản hồi tốc độ thực tế, tín hiệu được lọc nhiễu bằng bộ lọc Kalman.
4. Bộ điều khiển PID tạo tín hiệu điều khiển cho ESC driver nhằm đạt được tốc độ mục tiêu.
5. Hệ thống liên tục giám sát các IO bảo vệ (nắp đậy, lỗi động cơ) và phản hồi trạng thái về giao diện người dùng.

## Biên dịch và nạp firmware

### Yêu cầu

- **Keil MDK-ARM** phiên bản 5 trở lên
- Gói thiết bị STM32F1 (Device Family Pack)
- ST-Link/V2 hoặc tương đương

### Các bước thực hiện

1. Cài đặt Keil MDK-ARM và gói thiết bị STM32F1.
2. Mở dự án tại `firmware/MDK-ARM/spin_coating.uvprojx`.
3. Kiểm tra cấu hình chân trong `board_config/Inc/pinmap.h` nếu có thay đổi phần cứng.
4. Build dự án để tạo file nhị phân tại `firmware/MDK-ARM/Objects/project.axf`.
5. Nạp firmware bằng ST-Link Utility hoặc trực tiếp từ Keil thông qua ST-Link/V2.

## Cấu trúc thư mục chính

```text
firmware/
├── core/              # Hàm main.c, scheduler và các hook khởi tạo
├── drivers/           # Driver UART, delay, và các ngoại vi cơ bản
├── components/        # Module encoder, ESC driver, IO driver
├── control_algorithms/# Thuật toán PID và Kalman filter
├── modes/             # Lập trình chu kỳ phủ và kịch bản tốc độ
├── protocol_UART/     # Message, converter và parser giao thức UART
└── MDK-ARM/           # Dự án Keil và file cấu hình

host_apps/
├── src/               # Ứng dụng C++ với LVGL để cấu hình và giám sát
└── lib/               # Thư viện màn hình và giao diện người dùng
```

## Các chế độ hoạt động

Firmware hỗ trợ nhiều chế độ phủ khác nhau:

- **Analog Mode**: Điều khiển tốc độ theo tín hiệu analog đầu vào
- **Digital Mode**: Điều khiển tốc độ theo giá trị số được cài đặt trước
- **Ramp Mode**: Tăng/giảm tốc độ theo đường dốc được định nghĩa
- **Coater Mode**: Chu trình phủ tự động với các bước được lập trình sẵn

Chi tiết về từng chế độ có thể tham khảo tại thư mục [`firmware/docs_example/`](firmware/docs_example/).

## Giao thức UART

Giao tiếp giữa firmware và ứng dụng host sử dụng giao thức UART với các đặc điểm:

- Baudrate: 115200 bps (cấu hình trong `uart.c`)
- Frame format: Start byte + Command + Data + Checksum
- Hỗ trợ các lệnh: đọc/ghi tham số, bắt đầu/dừng, đọc trạng thái

Chi tiết giao thức xem tại [`firmware/docs_example/uart_protocol.md`](firmware/docs_example/uart_protocol.md).

## Kế hoạch phát triển

- [ ] Bổ sung unit test cho bộ điều khiển PID và parser UART
- [ ] Mở rộng profile phủ nhiều bước với lịch trình thời gian thực
- [ ] Đồng bộ thông số hiệu chuẩn giữa firmware và host app bằng checksum
- [ ] Thêm chế độ ghi log dữ liệu vào SD card
- [ ] Hỗ trợ cập nhật firmware qua UART (bootloader)

## Đóng góp

Mọi đóng góp đều được hoan nghênh! Vui lòng:

1. Fork repository này
2. Tạo branch cho tính năng mới (`git checkout -b feature/TinhNangMoi`)
3. Commit các thay đổi (`git commit -m 'Thêm tính năng mới'`)
4. Push lên branch (`git push origin feature/TinhNangMoi`)
5. Tạo Pull Request

## Liên hệ

- Sử dụng tab [Issues](https://github.com/haihbv/Spin-Coater/issues) để báo lỗi hoặc đề xuất tính năng mới
- Email tác giả: xem thông tin trong các commit
- Documentation: Tham khảo thêm tại thư mục [`firmware/docs_example/`](firmware/docs_example/)
