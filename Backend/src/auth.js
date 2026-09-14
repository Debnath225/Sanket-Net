import crypto from "node:crypto";

const base64url = (value) => Buffer.from(value).toString("base64url");
const json = (value) => base64url(JSON.stringify(value));

const signatureFor = (value, secret) =>
  crypto.createHmac("sha256", secret).update(value).digest("base64url");

const safeEqual = (first, second) => {
  const firstBuffer = Buffer.from(first);
  const secondBuffer = Buffer.from(second);
  return (
    firstBuffer.length === secondBuffer.length &&
    crypto.timingSafeEqual(firstBuffer, secondBuffer)
  );
};

const hashPassword = (password, salt = crypto.randomBytes(16).toString("hex")) =>
  new Promise((resolve, reject) =>
    crypto.pbkdf2(password, salt, 120_000, 64, "sha512", (error, derivedKey) => {
      if (error) reject(error);
      else resolve(`pbkdf2$${salt}$${derivedKey.toString("hex")}`);
    }),
  );

const verifyPassword = async (password, storedHash) => {
  const [algorithm, salt, expected] = String(storedHash || "").split("$");
  if (algorithm !== "pbkdf2" || !salt || !expected) return false;
  return safeEqual((await hashPassword(password, salt)).split("$")[2], expected);
};

export function createAuth(config) {
  const authenticate = (username, password) =>
    typeof username === "string" &&
    typeof password === "string" &&
    safeEqual(username, config.authUsername) &&
    safeEqual(password, config.authPassword);

  const issueToken = (username = config.authUsername, role = "ADMIN") => {
    const now = Math.floor(Date.now() / 1000);
    const payload = {
      sub: username,
      role,
      iat: now,
      exp: now + config.authTokenTtlSeconds,
    };
    const unsigned = `${json({ alg: "HS256", typ: "JWT" })}.${json(payload)}`;
    return {
      token: `${unsigned}.${signatureFor(unsigned, config.authTokenSecret)}`,
      expiresAt: new Date(payload.exp * 1000).toISOString(),
    };
  };

  const verifyToken = (token) => {
    if (typeof token !== "string") return null;
    const parts = token.split(".");
    if (parts.length !== 3) return null;
    const [header, payload, signature] = parts;
    const unsigned = `${header}.${payload}`;
    if (!safeEqual(signature, signatureFor(unsigned, config.authTokenSecret)))
      return null;
    try {
      const claims = JSON.parse(Buffer.from(payload, "base64url").toString());
      if (
        !["ADMIN", "OPERATOR", "VIEWER"].includes(claims.role) ||
        typeof claims.sub !== "string" ||
        !Number.isFinite(claims.exp) ||
        claims.exp <= Math.floor(Date.now() / 1000)
      )
        return null;
      return claims;
    } catch {
      return null;
    }
  };

  const requireAuth = (request, response, next) => {
    const match = request.headers.authorization?.match(/^Bearer (.+)$/i);
    const session = verifyToken(match?.[1]);
    if (!session)
      return response.status(401).json({ error: "authentication_required" });
    request.session = session;
    next();
  };

  return { authenticate, hashPassword, verifyPassword, issueToken, verifyToken, requireAuth };
}
