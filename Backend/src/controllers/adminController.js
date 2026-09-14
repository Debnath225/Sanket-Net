const isAdmin = (request) => request.session?.role === "ADMIN";
const validRole = (value) => ["ADMIN", "OPERATOR", "VIEWER"].includes(value);

export function createAdminController(store, config) {
  return {
    requireAdmin: (request, response, next) => isAdmin(request) ? next() : response.status(403).json({ error: "admin_required" }),
    status: (_request, response) => response.json({ persistence: store.persistenceMode, mqttConfigured: Boolean(config.mqttUrl), llmConfigured: Boolean(config.llmApiKey), registrationEnabled: config.allowRegistration, at: new Date().toISOString() }),
    users: async (_request, response, next) => { try { response.json(await store.listUsers()); } catch (error) { next(error); } },
    updateUser: async (request, response, next) => {
      const { role, active } = request.body || {};
      if (role !== undefined && !validRole(role)) return response.status(400).json({ error: "invalid_role" });
      if (active !== undefined && typeof active !== "boolean") return response.status(400).json({ error: "invalid_active" });
      if (request.params.username === request.session.sub && active === false) return response.status(400).json({ error: "cannot_deactivate_self" });
      try { const user = await store.updateUser(request.params.username, { ...(role !== undefined ? { role } : {}), ...(active !== undefined ? { active } : {}) }); response.json(user); } catch (error) { if (error.code === "P2025") return response.status(404).json({ error: "user_not_found" }); next(error); }
    },
    conversations: async (_request, response, next) => { try { response.json(await store.listConversations("", true)); } catch (error) { next(error); } },
    deleteConversation: async (request, response, next) => { try { if (!await store.deleteConversation(request.params.id, "", true)) return response.status(404).json({ error: "conversation_not_found" }); return response.status(204).end(); } catch (error) { return next(error); } },
  };
}
