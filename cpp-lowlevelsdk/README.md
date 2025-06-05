# C++ Sample application for Observable Space Low Level SDK

---

### Setup

When connecting to the low level SDK it is recommended to trust the Observable Space root certificate.

This can be done by running the following command:

```bash
openssl s_client -showcerts -connect <host>:50099 </dev/null 2>/dev/null | openssl x509 -outform PEM > observable_space_root.pem
```

Then, add the certificate to your trusted certificates:

```bash
sudo cp observable_space_root.pem /usr/local/share/ca-certificates/
sudo update-ca-certificates
```

If you cannot, or do not want to, trust the root certificate, you can also present the certificate when connecting with the SDK.

### Build

To build the sample application, you need to have the following dependencies installed:
- CMake
- C++ compiler (g++, clang++, etc.)

To build the sample application, run the following commands:

```bash
mkdir build
cd build
cmake ..
make
```

### Running

```
./grpc_client <controller_ip> --controller-id <controller_id> --cert-dir <controller_certs>
```
