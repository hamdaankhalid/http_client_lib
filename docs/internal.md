# Internal Eng Doc
## Okay so this is where I word vomit http/1.1 and then http/2.0

### HTTP Client Class
- Client should be able to have multiple TCP connections it can manage in a threadsafe manner.
- Client hides away all details under the hood of connection pooling, multiplexing or whatever. All it does it Request, Response sync calls.
- Client gets data type Request, and sends data type Response.

### HTTP Request
- Request obj is a data wrapper that can go from C++ obj -> Byte representation we can send over TCP!

### HTTP Response
- Response obj is a data wrapper that can be constructed from buffer into C++ obj. Everything that is till end of header section should be read into the buffer. At this point it chooses whether or not a body exists based on header metadata and is aware if response body will exist or not. It will provide different ways to read the response body. 

### The HTTP Connection Abstraction
- For client do this, it needs the ability to get a connection from a pool, this connection needs a way to be able to send, and read.
- Connection would be HTTP/1.1 and HTTP/2.0 specific impls.
