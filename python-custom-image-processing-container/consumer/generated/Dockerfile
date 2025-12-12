FROM python:3.13 AS builder

WORKDIR /usr/src/app

RUN python3 -m venv /venv
ENV PATH="/venv/bin:$PATH"

RUN pip install --upgrade pip

COPY . .
RUN pip install --no-cache-dir -r requirements.txt && \
    pip install --no-cache-dir .

FROM python:3.13 AS service
WORKDIR /root/app/site-packages
COPY --from=builder /venv /venv
ENV PATH=/venv/bin:$PATH

CMD ["uvicorn", "openapi_server.main:app", "--host", "0.0.0.0", "--port", "8099"]
