using System;
using System.Threading;
using System.Threading.Tasks;

namespace FairgroundLights
{
    class Program
    {
        const int StripCount = 2;
        const int LedCount = 60;
        const int BytesPerPixel = 4;  // RGBW
        const int HeaderSize = 1;

        private static DateTime _start = DateTime.UtcNow;

        static async Task Main(string[] args)
        {
            Console.WriteLine("Write pixels to ESP32");

            using (var cts = new CancellationTokenSource())
            {
                // when cancel is pressed
                Console.CancelKeyPress += (s, e) =>
                {
                    cts.Cancel();
                    e.Cancel = true; // don't terminate the app, just let it finish gracefully
                };

                using (var cs = new System.Net.WebSockets.ClientWebSocket())
                {
                    // don't forget to change the IP
                    await cs.ConnectAsync(new Uri("ws://192.168.8.222:1337"), cts.Token);

                    var buffer = new byte[HeaderSize + StripCount * LedCount * BytesPerPixel]; // 2 strips, 60 leds, 4 bytes per pixel (RGBW)

                    // while the app is running, write the pixels to the ESP32
                    while (!cts.IsCancellationRequested)
                    {
                        UpdateBuffer(buffer);
                        await cs.SendAsync(new ArraySegment<byte>(buffer), System.Net.WebSockets.WebSocketMessageType.Binary, true, cts.Token);
                    }
                }
            }
        }

        private static void UpdateBuffer(byte[] buffer)
        {
            int offset = 0;
            
            // header byte:
            buffer[offset++] = 128 + 120;

            int pixel = (int)(DateTime.UtcNow - _start).TotalSeconds;

            for (int s = 0; s < StripCount; s++)
                for (int l = 0; l < LedCount; l++)
                {
                    buffer[offset++] = (pixel % 3 == 0) ? (byte)255 : (byte)0; // R
                    buffer[offset++] = (pixel % 3 == 1) ? (byte)255 : (byte)0; // G
                    buffer[offset++] = (pixel % 3 == 2) ? (byte)255 : (byte)0; // B
                    buffer[offset++] = (byte)0; // W
                    pixel++;
                }
        }
    }
}
