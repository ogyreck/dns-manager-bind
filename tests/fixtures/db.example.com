$TTL 3600
@   IN  SOA ns1.example.com. admin.example.com. (
        2026050501  ; serial
        3600        ; refresh
        900         ; retry
        604800      ; expire
        300         ; minimum
        )
@       IN  NS      ns1.example.com.
@       IN  NS      ns2.example.com.
ns1     IN  A       192.168.1.1
ns2     IN  A       192.168.1.2
@       IN  A       192.168.1.10
www     IN  A       192.168.1.10
mail    IN  A       192.168.1.20
@       IN  AAAA    2001:db8::1
www     IN  CNAME   @
@       IN  MX      10 mail.example.com.
@       IN  TXT     "v=spf1 a mx ~all"
1       IN  PTR     ns1.example.com.
