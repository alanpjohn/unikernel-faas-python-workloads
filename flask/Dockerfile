FROM python:3.10.11 AS base

WORKDIR /app

COPY requirements.txt /app

RUN pip3 install -r requirements.txt --no-cache-dir -t /app

FROM scratch

COPY --from=base /usr/local/lib/python3.10 /lib/python3.10
COPY --from=base /app /app
COPY ./app/wrapper.py /app/wrapper.py
