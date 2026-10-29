import axios from "axios";
import Constants from "expo-constants";
import { Drink, UserProfile } from "../types";

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

// Désactivation du mode Test pour taper sur le vrai backend C++
export const USE_MOCK = false; 
export const API_BASE_URL = `http://${getBackendHost()}:8080/api`;

export const apiClient = axios.create({
    baseURL: API_BASE_URL,
    headers: { "Content-Type": "application/json" },
    timeout: 5000,
});

export let mockUser: UserProfile = { id: 1, name: "Erwann", balance: 8.4 };
export const mockDrinks: Drink[] = [{ id: 1, name: "Espresso", category: "Coffee", stock: 10, price: 0.8 }];