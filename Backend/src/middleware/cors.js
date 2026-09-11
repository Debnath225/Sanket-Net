export function cors(origins) {
  return (request, response, next) => {
    const origin = request.headers.origin;
    if (origin && origins.includes(origin)) {
      response.setHeader("Access-Control-Allow-Origin", origin);
      response.setHeader("Vary", "Origin");
    }
    response.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    response.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    if (request.method === "OPTIONS") return response.sendStatus(204);
    next();
  };
}
