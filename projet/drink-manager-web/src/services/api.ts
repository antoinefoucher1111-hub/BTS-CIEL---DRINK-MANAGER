import axios from "axios";
import Constants from "expo-constants";
import {
    BiometricAuthResponse,
    Drink,
    HealthResponse,
    HistoryItem,
    OrderResponse,
    RechargeResponse,
    UserProfile,
} from "../types";

const getBackendHost = () => {
    if (typeof window !== "undefined" && window.location?.hostname) {
        return window.location.hostname;
    }
    const debuggerHost = Constants.expoConfig?.hostUri;
    if (debuggerHost) {
        return debuggerHost.split(":")[0];
    }
    return "localhost";
};

// Connexion directe au backend C++ (port 8080)
export const USE_MOCK = false;
export const API_BASE_URL = `http://${getBackendHost()}:8080/api`;

export const apiClient = axios.create({
    baseURL: API_BASE_URL,
    headers: { "Content-Type": "application/json" },
    timeout: 5000,
});

// ---- Session (jeton délivré par le serveur C++ après le code biométrique) ----
const TOKEN_KEY = "dm_token";
const USER_KEY = "dm_user_id";
const ADMIN_TOKEN_KEY = "dm_admin_token";
let authToken: string | null = null;
let adminToken: string | null = null;
let onUnauthorized: (() => void) | null = null;

const storage = () => (typeof window !== "undefined" ? window.sessionStorage : undefined);

export const adminSession = {
    load: (): string | null => {
        const t = storage()?.getItem(ADMIN_TOKEN_KEY) || null;
        adminToken = t;
        return t;
    },
    save: (token: string) => {
        adminToken = token;
        storage()?.setItem(ADMIN_TOKEN_KEY, token);
    },
    clear: () => {
        adminToken = null;
        storage()?.removeItem(ADMIN_TOKEN_KEY);
    },
};

const adminConfig = () => (adminToken ? { headers: { Authorization: `Bearer ${adminToken}` } } : {});

export const session = {
    load: (): { token: string; userId: number } | null => {
        const t = storage()?.getItem(TOKEN_KEY);
        const u = Number(storage()?.getItem(USER_KEY));
        if (t && u > 0) {
            authToken = t;
            return { token: t, userId: u };
        }
        return null;
    },
    save: (token: string, userId: number) => {
        authToken = token;
        storage()?.setItem(TOKEN_KEY, token);
        storage()?.setItem(USER_KEY, String(userId));
    },
    clear: () => {
        authToken = null;
        storage()?.removeItem(TOKEN_KEY);
        storage()?.removeItem(USER_KEY);
    },
    onUnauthorized: (cb: () => void) => {
        onUnauthorized = cb;
    },
};

apiClient.interceptors.request.use((config) => {
    if (authToken && !config.headers.get("Authorization")) {
        config.headers.set("Authorization", `Bearer ${authToken}`);
    }
    return config;
});

apiClient.interceptors.response.use(
    (r) => r,
    (error) => {
        if (error?.response?.status === 401 && authToken) {
            session.clear();
            onUnauthorized?.();
        }
        return Promise.reject(error);
    }
);

export const api = {
    getHealth: async (): Promise<HealthResponse> => {
        const { data } = await apiClient.get<HealthResponse>("/health");
        return data;
    },
    adminLogin: async (username: string, password: string): Promise<{ status: string; role: string; token: string }> => {
        const { data } = await apiClient.post<{ status: string; role: string; token: string }>("/admin/login", {
            username,
            password,
        });
        return data;
    },
    getUsers: async (): Promise<UserProfile[]> => {
        const { data } = await apiClient.get<UserProfile[]>("/users", adminConfig());
        return data;
    },
    getUser: async (userId: number | string): Promise<UserProfile> => {
        const { data } = await apiClient.get<UserProfile>(`/users/${userId}`);
        return data;
    },
    createUser: async (name: string, balance = 0, accessPrint = 0): Promise<UserProfile> => {
        const { data } = await apiClient.post<UserProfile>(
            "/users",
            { name, balance, access_print: accessPrint },
            adminConfig()
        );
        return data;
    },
    updateUserName: async (userId: number | string, name: string): Promise<void> => {
        await apiClient.put(`/users/${userId}`, { name });
    },
    updateFingerprint: async (userId: number | string, accessPrint: number): Promise<void> => {
        await apiClient.put(`/users/${userId}/fingerprint`, { access_print: accessPrint }, adminConfig());
    },
    biometricScan: async (fingerprintId: number): Promise<BiometricAuthResponse> => {
        const { data } = await apiClient.post<BiometricAuthResponse>("/auth/biometric", {
            fingerprint_id: fingerprintId,
        });
        return data;
    },
    verifyAuthCode: async (code: string): Promise<UserProfile & { token: string }> => {
        const { data } = await apiClient.post<UserProfile & { token: string }>("/auth/verify-code", { code });
        return data;
    },
    getDrinks: async (): Promise<Drink[]> => {
        const { data } = await apiClient.get<Drink[]>("/drinks");
        return data;
    },
    placeOrder: async (userId: number, drinkId: number, quantity = 1): Promise<OrderResponse> => {
        const { data } = await apiClient.post<OrderResponse>("/orders", {
            user_id: userId,
            drink_id: drinkId,
            quantity,
        });
        return data;
    },
    recharge: async (userId: number, amount: number): Promise<RechargeResponse> => {
        const { data } = await apiClient.post<RechargeResponse>("/recharge", {
            user_id: userId,
            amount,
        });
        return data;
    },
    getUserHistory: async (userId: number | string): Promise<HistoryItem[]> => {
        const { data } = await apiClient.get<HistoryItem[]>(`/users/${userId}/history`);
        return data;
    },
    getHistoryXmlUrl: (userId?: number | string): string => {
        return userId
            ? `${API_BASE_URL}/history/xml?user_id=${userId}`
            : `${API_BASE_URL}/history/xml`;
    },
    getHistoryXml: async (userId?: number | string): Promise<string> => {
        const url = userId ? `/history/xml?user_id=${userId}` : "/history/xml";
        const { data } = await apiClient.get<string>(url, { responseType: "text" });
        return data;
    },
};

export let mockUser: UserProfile = { id: 1, name: "Lucas", balance: 25.5 };
export const mockDrinks: Drink[] = [
    { id: 1, name: "Coca-Cola", category: "Soda", stock: 120, price: 1.8 },
];
