# libutils

## httpd - web server implementation example
To build this example, execute `make httpd`.

The included `httpd.ini` contains a starting configuration for a listener on port 8000 with a minimal set of mime-types.  The server will default to `none` if there is no extension on the requested file or if there is no defined mime-type for the extension that is present.

This server is primarily designed for simple tasks such as minimal (html only) websites and the quick sharing of data.  The most complex feature it implements is the ability to resume downloads with, e.g., `wget -c <URL>`.

The secondary design goal is for monitoring exploit attempts by logging the requests.  This is a pseudo-honeypot that can be useful for gaining insight to the types of attacks in common use.  Additional isolation is required, but many of the implementation decisions are based on potential for abuse, such as timeouts and data limits.

The logging can be insightful, for example:
```
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - Connected
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - POST /boaform/admin/formLogin HTTP/1.1
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - HOST = nnn.nnn.nnn.nnn:pppp
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - USER-AGENT = Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:71.0) Gecko/20100101 Firefox/71.0
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - ACCEPT = text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - ACCEPT-LANGUAGE = en-GB,en;q=0.5
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - ACCEPT-ENCODING = gzip, deflate
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - CONTENT-TYPE = application/x-www-form-urlencoded
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - CONTENT-LENGTH = 29
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - ORIGIN = http://nnn.nnn.nnn.nnn:pppp
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - CONNECTION = keep-alive
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - REFERER = http://nnn.nnn.nnn.nnn:pppp/admin/login.asp
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - UPGRADE-INSECURE-REQUESTS = 1
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - username=admin&psd=Feefifofum
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - Response: 405
MMMM-YY-DD HH:MM:SS (UTC) - nnn.nnn.nnn.nnn:pppp - Disconnected
```

