# HTTP Server example

This example connects to the specified Wi-Fi network and launches an HTTP Server on port 80.

## Configuration and Uploading

1. Add your board to the `platformio.ini` if needed
2. Edit the `partitions.csv` file if needed
3. Specify your Wi-Fi network credentials
   1. By creating a file named `credentials.json` with the following format:
      ```json
      {
          "ssid": "<Network's SSID>",
          "passwd": "<Network's Password>"
      }
      ```
   2. Or you will be asked for credentials during the building process. In that case, build the example
      from the console by executing
      ```bash
      pio run -t upload
      # or
      pio run -t upload -t monitor
      ```
      **Note**: in CLion, if you directly upload the example (i.e. not from your system's console), you won't be able
      to input your credentials (as of 03.2024).
4. Upload Filesystem image:
   ```bash
   pio run -t uploadfs
   ```
5. Upload / monitor:
   ```bash
   pio device monitor
   # or
   pio run -t upload
   # or
   pio run -t upload -t monitor
   ```

## Endpoints

| Endpoint                           | Response description              | Notes                                     |
|------------------------------------|-----------------------------------|-------------------------------------------|
| `POST /api/echo`                   | The same data as in request body  | r/w in chunks, can be used to send files* |
| `POST /api/echo-txt`               | The same data as in request body  | text is expected                          |
| `GET  /api/hello/{username}`       | `Hello, $username`                |                                           |
| `GET  /api/hello/{name}/{surname}` | `Hello, $name $surname`           |                                           |
| `GET  /api/path/{path}*`           | `Path: $path`                     |                                           |
| `GET  /{file}*`                    | The content of the specified file | 404 in case of non-existent file          |

*: to test file transfer you can you the following command:

```bash
curl --data-binary @/path/to/file $ip:80/api/echo > file
```
