import { useCallback, useEffect, useMemo, useState } from "react";
import {
    ActivityIndicator,
    Platform,
    ScrollView,
    StyleSheet,
    Text,
    TextInput,
    TouchableOpacity,
    View,
} from "react-native";
import { adminSession, api, API_BASE_URL, session } from "./src/services/api";
import { Drink, HistoryItem, UserProfile } from "./src/types";

const COLORS = {
    background: "#FDF8F5",
    primary: "#6F4E37",
    primaryLight: "#8B5A2B",
    secondary: "#E6D5C3",
    textDark: "#3E2723",
    textMuted: "#8D6E63",
    white: "#FFFFFF",
    success: "#2E8B57",
    successBg: "#E8F5E9",
    danger: "#D32F2F",
    dangerBg: "#FFEBEE",
    warning: "#ED6C02",
    border: "#EFEBE9",
};

type TabType = "menu" | "history";

interface ToastMessage {
    type: "success" | "error";
    text: string;
}

const getCategoryIcon = (category: string, name: string): string => {
    const c = category.toLowerCase();
    const n = name.toLowerCase();
    if (c.includes("tea") || n.includes("thé")) return "🍵";
    if (c.includes("coffee") || n.includes("café") || n.includes("cappuccino") || n.includes("latte")) return "☕";
    if (c.includes("water")) return "💧";
    if (c.includes("beer")) return "🍺";
    if (c.includes("mocktail")) return "🍹";
    if (c.includes("energy")) return "⚡";
    if (c.includes("juice")) return "🧃";
    if (c.includes("hot")) return "🍫";
    return "🥤";
};

export default function App() {
    // userId === 0  =>  personne n'est connecté : l'écran de connexion biométrique est obligatoire
    const [userId, setUserId] = useState<number>(0);
    const [user, setUser] = useState<UserProfile | null>(null);
    const [drinks, setDrinks] = useState<Drink[]>([]);
    const [history, setHistory] = useState<HistoryItem[]>([]);

    const [loading, setLoading] = useState(false);
    const [apiConnected, setApiConnected] = useState<boolean>(false);
    const [activeTab, setActiveTab] = useState<TabType>("menu");
    const [toast, setToast] = useState<ToastMessage | null>(null);

    // Profil
    const [isEditingName, setIsEditingName] = useState(false);
    const [editedName, setEditedName] = useState("");

    // Rechargement personnalisé
    const [customTopUp, setCustomTopUp] = useState("");

    // Filtres catalogue
    const [selectedCategory, setSelectedCategory] = useState<string>("Toutes");
    const [searchQuery, setSearchQuery] = useState<string>("");
    const [busyDrinkId, setBusyDrinkId] = useState<number | null>(null);

    // Aperçu XML
    const [xmlPreview, setXmlPreview] = useState<string | null>(null);

    // Authentification biométrique (Code 6 chiffres)
    const [authCodeInput, setAuthCodeInput] = useState("");
    const [simFingerprintId, setSimFingerprintId] = useState("1");
    const [generatedSerialInfo, setGeneratedSerialInfo] = useState<string | null>(null);

    // Mode Admin (création utilisateur & liaison empreinte)
    const [adminView, setAdminView] = useState<"none" | "login" | "panel">("none");
    const [adminUsername, setAdminUsername] = useState("");
    const [adminPassword, setAdminPassword] = useState("");
    const [adminUsers, setAdminUsers] = useState<UserProfile[]>([]);
    const [adminLoading, setAdminLoading] = useState(false);
    const [newUserName, setNewUserName] = useState("");
    const [newUserBalance, setNewUserBalance] = useState("0");
    const [newUserFingerprint, setNewUserFingerprint] = useState("");
    const [editingFpUserId, setEditingFpUserId] = useState<number | null>(null);
    const [editingFpValue, setEditingFpValue] = useState("");

    const showToast = useCallback((type: "success" | "error", text: string) => {
        setToast({ type, text });
    }, []);

    useEffect(() => {
        if (!toast) return;
        const timer = setTimeout(() => setToast(null), 4000);
        return () => clearTimeout(timer);
    }, [toast]);

    // Session : l'application est inaccessible sans code biométrique valide
    useEffect(() => {
        session.onUnauthorized(() => {
            setUserId(0);
            setUser(null);
            showToast("error", "Session expirée : reconnectez-vous avec votre empreinte.");
        });
        const saved = session.load();
        if (saved) setUserId(saved.userId);
    }, [showToast]);

    const loadAllData = useCallback(async (targetUserId: number) => {
        try {
            setLoading(true);
            const [healthRes, userRes, drinksRes, historyRes] = await Promise.all([
                api.getHealth(),
                api.getUser(targetUserId),
                api.getDrinks(),
                api.getUserHistory(targetUserId),
            ]);
            setApiConnected(healthRes.status === "ok");
            setUser(userRes);
            setEditedName(userRes.name);
            setDrinks(drinksRes);
            setHistory(historyRes);
        } catch (err: any) {
            console.error("Erreur de chargement API C++:", err);
            if (err?.response?.status === 401 || err?.response?.status === 403) {
                session.clear();
                setUserId(0);
            } else {
                setApiConnected(false);
                showToast("error", "Impossible de joindre le serveur C++ (" + API_BASE_URL + ")");
            }
            setUser(null);
        } finally {
            setLoading(false);
        }
    }, [showToast]);

    useEffect(() => {
        if (userId > 0) loadAllData(userId);
    }, [userId, loadAllData]);

    const handleLogout = () => {
        session.clear();
        setUser(null);
        setUserId(0);
        setDrinks([]);
        setHistory([]);
        setXmlPreview(null);
        setAuthCodeInput("");
        setGeneratedSerialInfo(null);
    };

    const categories = useMemo(() => {
        const unique = Array.from(new Set(drinks.map((d) => d.category).filter(Boolean)));
        return ["Toutes", ...unique];
    }, [drinks]);

    const filteredDrinks = useMemo(() => {
        return drinks.filter((d) => {
            const matchCat = selectedCategory === "Toutes" || d.category === selectedCategory;
            const matchQuery =
                searchQuery.trim() === "" ||
                d.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
                d.category.toLowerCase().includes(searchQuery.toLowerCase());
            return matchCat && matchQuery;
        });
    }, [drinks, selectedCategory, searchQuery]);

    const handleSaveProfile = async () => {
        const trimmed = editedName.trim();
        if (!trimmed) {
            showToast("error", "Le nom ne peut pas être vide.");
            return;
        }
        try {
            await api.updateUserName(userId, trimmed);
            setUser((prev) => (prev ? { ...prev, name: trimmed } : null));
            setIsEditingName(false);
            showToast("success", "Profil mis à jour !");
        } catch {
            showToast("error", "Échec de la mise à jour du profil.");
        }
    };

    const handleTopUp = async (amount: number) => {
        if (!user || amount <= 0 || Number.isNaN(amount)) {
            showToast("error", "Montant invalide.");
            return;
        }
        try {
            const res = await api.recharge(userId, amount);
            setUser((prev) => (prev ? { ...prev, balance: res.new_balance } : null));
            setCustomTopUp("");
            showToast("success", `Compte crédité de +${amount.toFixed(2)} €`);
        } catch (err: any) {
            const msg = err?.response?.data?.error || "Impossible de créditer le compte.";
            showToast("error", msg);
        }
    };

    const handleOrderDrink = async (drink: Drink) => {
        if (!user) return;
        if (drink.stock <= 0) {
            showToast("error", `${drink.name} est en rupture de stock.`);
            return;
        }
        if (user.balance < drink.price) {
            showToast("error", `Solde insuffisant : il manque ${(drink.price - user.balance).toFixed(2)} €.`);
            return;
        }
        try {
            setBusyDrinkId(drink.id);
            const res = await api.placeOrder(userId, drink.id, 1);
            setUser((prev) => (prev ? { ...prev, balance: res.new_balance } : null));
            setDrinks((prev) => prev.map((d) => (d.id === drink.id ? { ...d, stock: res.new_stock } : d)));
            const freshHistory = await api.getUserHistory(userId);
            setHistory(freshHistory);
            showToast("success", `Commande validée : 1 ${drink.name} ☕`);
        } catch (err: any) {
            const msg = err?.response?.data?.error || "Impossible de valider la commande.";
            showToast("error", msg);
        } finally {
            setBusyDrinkId(null);
        }
    };

    const handleExportXml = async () => {
        try {
            const xml = await api.getHistoryXml(userId);
            setXmlPreview(xml);
        } catch {
            showToast("error", "Export XML impossible.");
        }
    };

    const handleSimulateScan = async () => {
        const fpId = Number(simFingerprintId);
        if (!fpId || fpId < 1 || fpId > 127) {
            showToast("error", "ID empreinte invalide (1..127).");
            return;
        }
        try {
            const res = await api.biometricScan(fpId);
            setGeneratedSerialInfo(
                `Empreinte #${fpId} (${res.user.name}) -> Arduino: ${res.serial_command.trim()}`
            );
            setAuthCodeInput(res.code);
            showToast("success", `Code généré : ${res.code}`);
        } catch (err: any) {
            showToast("error", err?.response?.data?.error || "Empreinte non reconnue.");
        }
    };

    const handleVerifyCode = async () => {
        const code = authCodeInput.trim();
        if (!/^\d{6}$/.test(code)) {
            showToast("error", "Veuillez saisir un code à 6 chiffres.");
            return;
        }
        try {
            const authed = await api.verifyAuthCode(code);
            session.save(authed.token, authed.id);
            setAuthCodeInput("");
            setGeneratedSerialInfo(null);
            setUserId(authed.id);
            showToast("success", `Bienvenue ${authed.name} (authentifié par empreinte) !`);
        } catch (err: any) {
            showToast("error", err?.response?.data?.error || "Code invalide ou expiré.");
        }
    };

    const computeNextFreeFingerprint = (list: UserProfile[]): string => {
        const used = new Set(list.map((u) => u.access_print || 0).filter((n) => n > 0));
        for (let i = 1; i <= 127; i++) {
            if (!used.has(i)) return String(i);
        }
        return "";
    };

    const loadAdminUsers = useCallback(async () => {
        try {
            setAdminLoading(true);
            const list = await api.getUsers();
            setAdminUsers(list);
            setNewUserFingerprint((prev) => prev || computeNextFreeFingerprint(list));
        } catch (err: any) {
            if (err?.response?.status === 401 || err?.response?.status === 403) {
                adminSession.clear();
                setAdminView("login");
                showToast("error", "Session admin expirée. Reconnectez-vous.");
            } else {
                showToast("error", "Impossible de charger les utilisateurs.");
            }
        } finally {
            setAdminLoading(false);
        }
    }, [showToast]);

    const handleOpenAdmin = () => {
        if (adminSession.load()) {
            setAdminView("panel");
            loadAdminUsers();
        } else {
            setAdminView("login");
        }
    };

    const handleAdminLogin = async () => {
        try {
            setAdminLoading(true);
            const res = await api.adminLogin(adminUsername.trim(), adminPassword);
            adminSession.save(res.token);
            setAdminPassword("");
            setAdminView("panel");
            showToast("success", "Mode administrateur activé.");
            await loadAdminUsers();
        } catch (err: any) {
            showToast("error", err?.response?.data?.error || "Identifiants administrateur incorrects.");
        } finally {
            setAdminLoading(false);
        }
    };

    const handleAdminLogout = () => {
        adminSession.clear();
        setAdminView("none");
        setAdminUsername("");
        setAdminPassword("");
        setEditingFpUserId(null);
    };

    const handleAdminCreateUser = async () => {
        const name = newUserName.trim();
        const balance = Number(newUserBalance.replace(",", ".")) || 0;
        const fp = newUserFingerprint.trim() === "" ? 0 : Number(newUserFingerprint.trim());
        if (!name) {
            showToast("error", "Veuillez saisir un nom d'utilisateur.");
            return;
        }
        if (Number.isNaN(balance) || balance < 0) {
            showToast("error", "Solde initial invalide.");
            return;
        }
        if (Number.isNaN(fp) || fp < 0 || fp > 127) {
            showToast("error", "ID d'empreinte invalide (0 à 127).");
            return;
        }
        try {
            setAdminLoading(true);
            const created = await api.createUser(name, balance, fp);
            const updatedList = [...adminUsers, created];
            setAdminUsers(updatedList);
            setNewUserName("");
            setNewUserBalance("0");
            setNewUserFingerprint(computeNextFreeFingerprint(updatedList));
            showToast(
                "success",
                `Compte #${created.id} (${created.name}) créé` + (fp > 0 ? ` avec empreinte #${fp} !` : " !")
            );
        } catch (err: any) {
            showToast("error", err?.response?.data?.error || "Impossible de créer l'utilisateur.");
        } finally {
            setAdminLoading(false);
        }
    };

    const handleAdminSaveFingerprint = async (targetUserId: number) => {
        const fp = editingFpValue.trim() === "" ? 0 : Number(editingFpValue.trim());
        if (Number.isNaN(fp) || fp < 0 || fp > 127) {
            showToast("error", "ID d'empreinte invalide (0 à 127).");
            return;
        }
        try {
            setAdminLoading(true);
            await api.updateFingerprint(targetUserId, fp);
            setAdminUsers((prev) =>
                prev.map((u) => (u.id === targetUserId ? { ...u, access_print: fp } : u))
            );
            setEditingFpUserId(null);
            showToast("success", fp > 0 ? `Empreinte #${fp} liée au compte #${targetUserId}` : `Empreinte dissociée`);
        } catch (err: any) {
            showToast("error", err?.response?.data?.error || "Échec de la liaison d'empreinte.");
        } finally {
            setAdminLoading(false);
        }
    };

    // ---------- Mode Admin (connexion ou panneau gestion utilisateurs) ----------
    if (adminView === "login") {
        return (
            <ScrollView style={styles.container} contentContainerStyle={[styles.scrollContent, { justifyContent: "center", flexGrow: 1 }]}>
                {toast && (
                    <View style={[styles.toast, toast.type === "success" ? styles.toastSuccess : styles.toastError]}>
                        <Text style={[styles.toastText, { color: toast.type === "success" ? COLORS.success : COLORS.danger }]}>
                            {toast.text}
                        </Text>
                    </View>
                )}
                <View style={[styles.card, { alignItems: "center" }]}>
                    <Text style={{ fontSize: 44 }}>⚙️</Text>
                    <Text style={[styles.headerName, { marginTop: 8, textAlign: "center" }]}>Mode Administrateur</Text>
                    <Text style={[styles.greeting, { textAlign: "center", marginTop: 6, marginBottom: 16 }]}>
                        Création de comptes et liaison des empreintes
                    </Text>
                    <TextInput
                        style={[styles.input, { width: "100%" }]}
                        value={adminUsername}
                        onChangeText={setAdminUsername}
                        autoCapitalize="none"
                        placeholder="Identifiant (admin)"
                    />
                    <TextInput
                        style={[styles.input, { width: "100%" }]}
                        value={adminPassword}
                        onChangeText={setAdminPassword}
                        secureTextEntry
                        placeholder="Mot de passe"
                        onSubmitEditing={handleAdminLogin}
                    />
                    <TouchableOpacity
                        style={[styles.btnPrimary, { width: "100%" }]}
                        onPress={handleAdminLogin}
                        disabled={adminLoading}
                    >
                        <Text style={styles.btnPrimaryText}>
                            {adminLoading ? "Connexion..." : "Se connecter en Admin"}
                        </Text>
                    </TouchableOpacity>
                    <TouchableOpacity
                        style={[styles.btnSecondary, { width: "100%", marginTop: 10, alignItems: "center" }]}
                        onPress={() => setAdminView("none")}
                    >
                        <Text style={styles.btnSecondaryText}>← Retour à la connexion biométrique</Text>
                    </TouchableOpacity>
                </View>
            </ScrollView>
        );
    }

    if (adminView === "panel") {
        return (
            <ScrollView style={styles.container} contentContainerStyle={styles.scrollContent}>
                {toast && (
                    <View style={[styles.toast, toast.type === "success" ? styles.toastSuccess : styles.toastError]}>
                        <Text style={[styles.toastText, { color: toast.type === "success" ? COLORS.success : COLORS.danger }]}>
                            {toast.text}
                        </Text>
                    </View>
                )}

                <View style={styles.header}>
                    <View>
                        <Text style={styles.greeting}>Drink Manager • Administration</Text>
                        <Text style={styles.headerName}>⚙️ Mode Admin</Text>
                    </View>
                    <View style={styles.headerActions}>
                        <TouchableOpacity style={styles.profileIconBtn} onPress={loadAdminUsers}>
                            <Text style={{ fontSize: 16 }}>↻</Text>
                        </TouchableOpacity>
                        <TouchableOpacity style={styles.profileIconBtn} onPress={handleAdminLogout}>
                            <Text style={{ fontSize: 16 }}>🚪</Text>
                        </TouchableOpacity>
                    </View>
                </View>

                {/* Formulaire de création d'utilisateur */}
                <View style={[styles.card, styles.editCard]}>
                    <Text style={styles.cardTitle}>➕ Créer un nouvel utilisateur</Text>
                    <Text style={{ fontSize: 12, color: COLORS.textMuted, marginTop: 4, marginBottom: 12 }}>
                        Associez directement un ID d'empreinte (1..127) correspondant à celui enregistré sur l'Arduino.
                    </Text>

                    <Text style={styles.inputLabel}>Nom complet</Text>
                    <TextInput
                        style={styles.input}
                        value={newUserName}
                        onChangeText={setNewUserName}
                        placeholder="Ex: Antoine"
                    />

                    <View style={{ flexDirection: "row", gap: 10 }}>
                        <View style={{ flex: 1 }}>
                            <Text style={styles.inputLabel}>Solde initial (€)</Text>
                            <TextInput
                                style={styles.input}
                                value={newUserBalance}
                                onChangeText={setNewUserBalance}
                                keyboardType="numeric"
                                placeholder="0"
                            />
                        </View>
                        <View style={{ flex: 1 }}>
                            <Text style={styles.inputLabel}>ID Empreinte (1..127)</Text>
                            <TextInput
                                style={styles.input}
                                value={newUserFingerprint}
                                onChangeText={(t) => setNewUserFingerprint(t.replace(/\D/g, ""))}
                                keyboardType="numeric"
                                placeholder="Ex: 31"
                            />
                        </View>
                    </View>

                    <TouchableOpacity
                        style={[styles.btnPrimary, { width: "100%", marginTop: 4 }]}
                        onPress={handleAdminCreateUser}
                        disabled={adminLoading}
                    >
                        <Text style={styles.btnPrimaryText}>
                            {adminLoading ? "Création..." : "Créer l'utilisateur"}
                        </Text>
                    </TouchableOpacity>
                </View>

                {/* Liste des utilisateurs & empreintes */}
                <View style={styles.card}>
                    <Text style={[styles.cardTitle, { marginBottom: 12 }]}>
                        👥 Utilisateurs enregistrés ({adminUsers.length})
                    </Text>
                    {adminUsers.map((u) => {
                        const isEditing = editingFpUserId === u.id;
                        return (
                            <View
                                key={u.id}
                                style={{
                                    paddingVertical: 10,
                                    borderBottomWidth: 1,
                                    borderBottomColor: COLORS.border,
                                }}
                            >
                                <View style={{ flexDirection: "row", justifyContent: "space-between", alignItems: "center" }}>
                                    <View style={{ flex: 1 }}>
                                        <Text style={{ fontWeight: "700", color: COLORS.textDark }}>
                                            #{u.id} • {u.name}
                                        </Text>
                                        <Text style={{ fontSize: 12, color: COLORS.textMuted, marginTop: 2 }}>
                                            Solde : {u.balance.toFixed(2)} € •{" "}
                                            {u.access_print && u.access_print > 0
                                                ? `Empreinte #${u.access_print}`
                                                : "Aucune empreinte"}
                                        </Text>
                                    </View>
                                    <TouchableOpacity
                                        style={styles.btnSecondary}
                                        onPress={() => {
                                            if (isEditing) {
                                                setEditingFpUserId(null);
                                            } else {
                                                setEditingFpUserId(u.id);
                                                setEditingFpValue(u.access_print ? String(u.access_print) : "");
                                            }
                                        }}
                                    >
                                        <Text style={styles.btnSecondaryText}>
                                            {isEditing ? "Fermer" : "🔑 Empreinte"}
                                        </Text>
                                    </TouchableOpacity>
                                </View>
                                {isEditing && (
                                    <View style={[styles.createUserRow, { marginTop: 8 }]}>
                                        <TextInput
                                            style={[styles.input, { flex: 1, marginBottom: 0 }]}
                                            value={editingFpValue}
                                            onChangeText={(t) => setEditingFpValue(t.replace(/\D/g, ""))}
                                            keyboardType="numeric"
                                            placeholder="ID empreinte (0 = aucune, 1..127)"
                                        />
                                        <TouchableOpacity
                                            style={styles.btnPrimary}
                                            onPress={() => handleAdminSaveFingerprint(u.id)}
                                        >
                                            <Text style={styles.btnPrimaryText}>Enregistrer</Text>
                                        </TouchableOpacity>
                                    </View>
                                )}
                            </View>
                        );
                    })}
                </View>

                <TouchableOpacity
                    style={[styles.btnSecondary, { width: "100%", alignItems: "center", marginBottom: 24 }]}
                    onPress={handleAdminLogout}
                >
                    <Text style={styles.btnSecondaryText}>← Quitter le mode Admin</Text>
                </TouchableOpacity>
            </ScrollView>
        );
    }

    // ---------- Écran de connexion obligatoire ----------
    if (userId === 0) {
        return (
            <ScrollView style={styles.container} contentContainerStyle={[styles.scrollContent, { justifyContent: "center", flexGrow: 1 }]}>
                {toast && (
                    <View style={[styles.toast, toast.type === "success" ? styles.toastSuccess : styles.toastError]}>
                        <Text style={[styles.toastText, { color: toast.type === "success" ? COLORS.success : COLORS.danger }]}>
                            {toast.text}
                        </Text>
                    </View>
                )}
                <View style={[styles.card, { alignItems: "center" }]}>
                    <Text style={{ fontSize: 48 }}>🔐</Text>
                    <Text style={[styles.headerName, { marginTop: 8, textAlign: "center" }]}>Drink Manager</Text>
                    <Text style={[styles.greeting, { textAlign: "center", marginTop: 6 }]}>
                        1. Posez votre doigt sur le capteur de la borne{"\n"}
                        2. Saisissez ici le code à 6 chiffres affiché
                    </Text>
                    <TextInput
                        style={[styles.input, { width: "100%", marginTop: 20, textAlign: "center", fontSize: 28, letterSpacing: 8 }]}
                        value={authCodeInput}
                        onChangeText={(t) => setAuthCodeInput(t.replace(/\D/g, ""))}
                        keyboardType="numeric"
                        maxLength={6}
                        placeholder="••••••"
                        onSubmitEditing={handleVerifyCode}
                    />
                    <TouchableOpacity style={[styles.btnPrimary, { width: "100%" }]} onPress={handleVerifyCode}>
                        <Text style={styles.btnPrimaryText}>Se connecter</Text>
                    </TouchableOpacity>
                    <Text style={{ marginTop: 14, fontSize: 12, color: COLORS.textMuted, textAlign: "center" }}>
                        Le code est valable 2 minutes et ne sert qu'une seule fois.
                    </Text>
                    <TouchableOpacity
                        style={[styles.btnSecondary, { width: "100%", marginTop: 14, alignItems: "center" }]}
                        onPress={handleOpenAdmin}
                    >
                        <Text style={styles.btnSecondaryText}>⚙️ Mode Admin (créer un utilisateur)</Text>
                    </TouchableOpacity>
                </View>

                {__DEV__ && (
                    <View style={[styles.card, styles.editCard]}>
                        <Text style={styles.inputLabel}>Mode test : simulateur de capteur (sans Arduino)</Text>
                        <Text style={{ fontSize: 12, color: COLORS.textDark, marginBottom: 8 }}>
                            Nécessite le serveur lancé avec DM_ALLOW_SIM=1
                        </Text>
                        <View style={styles.createUserRow}>
                            <TextInput
                                style={[styles.input, { flex: 1, marginBottom: 0 }]}
                                value={simFingerprintId}
                                onChangeText={setSimFingerprintId}
                                keyboardType="numeric"
                                placeholder="ID empreinte (1..127)"
                            />
                            <TouchableOpacity style={styles.btnPrimary} onPress={handleSimulateScan}>
                                <Text style={styles.btnPrimaryText}>Simuler scan</Text>
                            </TouchableOpacity>
                        </View>
                        {generatedSerialInfo && (
                            <Text style={{ marginTop: 10, fontSize: 12, color: COLORS.textDark, fontWeight: "600" }}>
                                {generatedSerialInfo}
                            </Text>
                        )}
                    </View>
                )}
            </ScrollView>
        );
    }

    if (loading) {
        return (
            <View style={styles.center}>
                <ActivityIndicator size="large" color={COLORS.primary} />
                <Text style={styles.loadingText}>Connexion au serveur C++ et à la base SQLite...</Text>
            </View>
        );
    }

    if (!user) {
        return (
            <View style={styles.center}>
                <Text style={{ fontSize: 42 }}>📡</Text>
                <Text style={styles.errorTitle}>Utilisateur #{userId} ou serveur inaccessible</Text>
                <Text style={styles.errorSub}>
                    Vérifiez que le backend C++ tourne sur {API_BASE_URL}
                </Text>
                <TouchableOpacity style={[styles.btnPrimary, { marginTop: 20 }]} onPress={() => loadAllData(userId)}>
                    <Text style={styles.btnPrimaryText}>Réessayer</Text>
                </TouchableOpacity>
                <TouchableOpacity style={[styles.btnSecondary, { marginTop: 8 }]} onPress={handleLogout}>
                    <Text style={styles.btnSecondaryText}>Retour à la connexion</Text>
                </TouchableOpacity>
            </View>
        );
    }

    return (
        <ScrollView style={styles.container} contentContainerStyle={styles.scrollContent}>
            {/* Bandeau statut API */}
            <View style={styles.statusBar}>
                <View style={styles.statusBadge}>
                    <View style={[styles.statusDot, { backgroundColor: apiConnected ? COLORS.success : COLORS.danger }]} />
                    <Text style={styles.statusText}>
                        {apiConnected ? "API C++ & SQLite connectées" : "API hors ligne"}
                    </Text>
                </View>
                <TouchableOpacity style={styles.refreshBtn} onPress={() => loadAllData(userId)}>
                    <Text style={styles.refreshText}>↻ Actualiser</Text>
                </TouchableOpacity>
            </View>

            {/* Toast feedback */}
            {toast && (
                <View style={[styles.toast, toast.type === "success" ? styles.toastSuccess : styles.toastError]}>
                    <Text style={{ fontSize: 16 }}>
                        {toast.type === "success" ? "✅" : "⚠️"}
                    </Text>
                    <Text
                        style={[
                            styles.toastText,
                            { color: toast.type === "success" ? COLORS.success : COLORS.danger },
                        ]}
                    >
                        {toast.text}
                    </Text>
                </View>
            )}

            {/* Header */}
            <View style={styles.header}>
                <View>
                    <Text style={styles.greeting}>
                        Drink Manager • Compte #{user.id}
                        {user.access_print ? ` • Empreinte #${user.access_print}` : ""}
                    </Text>
                    <Text style={styles.headerName}>Bonjour, {user.name} 👋</Text>
                </View>
                <View style={styles.headerActions}>
                    <TouchableOpacity
                        style={styles.profileIconBtn}
                        onPress={() => setIsEditingName(!isEditingName)}
                    >
                        <Text style={{ fontSize: 18 }}>✏️</Text>
                    </TouchableOpacity>
                    <TouchableOpacity style={styles.profileIconBtn} onPress={handleLogout}>
                        <Text style={{ fontSize: 18 }}>🚪</Text>
                    </TouchableOpacity>
                </View>
            </View>

            {/* Edition du nom */}
            {isEditingName && (
                <View style={[styles.card, styles.editCard]}>
                    <Text style={styles.inputLabel}>Modifier votre nom</Text>
                    <TextInput
                        style={styles.input}
                        value={editedName}
                        onChangeText={setEditedName}
                        placeholder="Votre nom complet"
                    />
                    <View style={styles.editActions}>
                        <TouchableOpacity style={styles.btnSecondary} onPress={() => setIsEditingName(false)}>
                            <Text style={styles.btnSecondaryText}>Annuler</Text>
                        </TouchableOpacity>
                        <TouchableOpacity style={styles.btnPrimary} onPress={handleSaveProfile}>
                            <Text style={styles.btnPrimaryText}>Enregistrer</Text>
                        </TouchableOpacity>
                    </View>
                </View>
            )}

            {/* Carte Portefeuille */}
            <View style={styles.card}>
                <View style={styles.cardHeader}>
                    <Text style={{ fontSize: 16 }}>💳</Text>
                    <Text style={styles.cardTitle}>Mon Portefeuille</Text>
                </View>
                <Text style={[styles.balance, user.balance < 2.0 ? styles.balanceLow : null]}>
                    {user.balance.toFixed(2)} <Text style={styles.currency}>€</Text>
                </Text>
                <Text style={styles.subSectionTitle}>Rechargement rapide</Text>
                <View style={styles.topUpRow}>
                    {[2, 5, 10, 20].map((val) => (
                        <TouchableOpacity key={val} style={styles.topUpBtn} onPress={() => handleTopUp(val)}>
                            <Text style={styles.topUpBtnText}>+{val} €</Text>
                        </TouchableOpacity>
                    ))}
                </View>
                <View style={styles.customTopUpRow}>
                    <TextInput
                        style={[styles.input, { flex: 1, marginBottom: 0 }]}
                        value={customTopUp}
                        onChangeText={setCustomTopUp}
                        keyboardType="numeric"
                        placeholder="Montant personnalisé (€)"
                    />
                    <TouchableOpacity
                        style={styles.btnPrimary}
                        onPress={() => handleTopUp(Number(customTopUp.replace(",", ".")))}
                    >
                        <Text style={styles.btnPrimaryText}>Créditer</Text>
                    </TouchableOpacity>
                </View>
            </View>

            {/* Onglets Catalogue / Historique */}
            <View style={styles.tabsRow}>
                <TouchableOpacity
                    style={[styles.tabBtn, activeTab === "menu" && styles.tabBtnActive]}
                    onPress={() => setActiveTab("menu")}
                >
                    <Text style={[styles.tabText, activeTab === "menu" && styles.tabTextActive]}>
                        ☕ La Carte ({drinks.length})
                    </Text>
                </TouchableOpacity>
                <TouchableOpacity
                    style={[styles.tabBtn, activeTab === "history" && styles.tabBtnActive]}
                    onPress={() => setActiveTab("history")}
                >
                    <Text style={[styles.tabText, activeTab === "history" && styles.tabTextActive]}>
                        🧾 Historique ({history.length})
                    </Text>
                </TouchableOpacity>
            </View>

            {activeTab === "menu" ? (
                <View>
                    {/* Recherche + Filtres */}
                    <View style={styles.searchBox}>
                        <Text style={{ fontSize: 16 }}>🔍</Text>
                        <TextInput
                            style={styles.searchInput}
                            value={searchQuery}
                            onChangeText={setSearchQuery}
                            placeholder="Rechercher une boisson..."
                            placeholderTextColor={COLORS.textMuted}
                        />
                        {searchQuery.length > 0 && (
                            <TouchableOpacity onPress={() => setSearchQuery("")}>
                                <Text style={{ fontSize: 16, color: COLORS.textMuted }}>✕</Text>
                            </TouchableOpacity>
                        )}
                    </View>

                    <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.catRow}>
                        {categories.map((cat) => (
                            <TouchableOpacity
                                key={cat}
                                style={[styles.catChip, selectedCategory === cat && styles.catChipActive]}
                                onPress={() => setSelectedCategory(cat)}
                            >
                                <Text style={[styles.catChipText, selectedCategory === cat && styles.catChipTextActive]}>
                                    {cat}
                                </Text>
                            </TouchableOpacity>
                        ))}
                    </ScrollView>

                    <View style={styles.drinksContainer}>
                        {filteredDrinks.length === 0 ? (
                            <View style={styles.emptyBox}>
                                <Text style={styles.emptyText}>Aucune boisson ne correspond à votre recherche.</Text>
                            </View>
                        ) : (
                            filteredDrinks.map((d) => {
                                const outOfStock = d.stock <= 0;
                                const isBusy = busyDrinkId === d.id;
                                return (
                                    <View key={d.id} style={[styles.drinkCard, outOfStock && { opacity: 0.55 }]}>
                                        <View style={styles.drinkInfo}>
                                            <View style={styles.drinkIconContainer}>
                                                <Text style={{ fontSize: 22 }}>
                                                    {getCategoryIcon(d.category, d.name)}
                                                </Text>
                                            </View>
                                            <View>
                                                <Text style={styles.drinkName}>{d.name}</Text>
                                                <View style={styles.drinkMetaRow}>
                                                    <Text style={styles.drinkPrice}>{d.price.toFixed(2)} €</Text>
                                                    <Text style={styles.drinkCat}>{d.category}</Text>
                                                    <Text
                                                        style={[
                                                            styles.drinkStock,
                                                            outOfStock ? { color: COLORS.danger } : null,
                                                        ]}
                                                    >
                                                        {outOfStock ? "Rupture" : `Stock: ${d.stock}`}
                                                    </Text>
                                                </View>
                                            </View>
                                        </View>
                                        <TouchableOpacity
                                            style={[styles.orderBtn, outOfStock && { backgroundColor: COLORS.textMuted }]}
                                            disabled={outOfStock || isBusy}
                                            onPress={() => handleOrderDrink(d)}
                                        >
                                            {isBusy ? (
                                                <ActivityIndicator size="small" color={COLORS.white} />
                                            ) : (
                                                <Text style={{ color: COLORS.white, fontSize: 20, fontWeight: "700" }}>+</Text>
                                            )}
                                        </TouchableOpacity>
                                    </View>
                                );
                            })
                        )}
                    </View>
                </View>
            ) : (
                <View>
                    <View style={styles.historyHeader}>
                        <Text style={styles.sectionTitle}>Mes consommations</Text>
                        <TouchableOpacity style={styles.xmlBtn} onPress={handleExportXml}>
                            <Text style={styles.xmlBtnText}>⬇ Exporter XML</Text>
                        </TouchableOpacity>
                    </View>

                    {xmlPreview && (
                        <View style={styles.xmlBox}>
                            <View style={styles.xmlBoxHeader}>
                                <Text style={styles.xmlBoxTitle}>Export XML (généré par C++)</Text>
                                <TouchableOpacity onPress={() => setXmlPreview(null)}>
                                    <Text style={{ color: "#E6D5C3", fontWeight: "700", fontSize: 16 }}>✕</Text>
                                </TouchableOpacity>
                            </View>
                            <Text style={styles.xmlCode}>{xmlPreview}</Text>
                        </View>
                    )}

                    {history.length === 0 ? (
                        <View style={styles.emptyBox}>
                            <Text style={styles.emptyText}>Aucun achat enregistré pour le moment.</Text>
                        </View>
                    ) : (
                        <View style={styles.drinksContainer}>
                            {history.map((item) => (
                                <View key={item.id} style={styles.historyCard}>
                                    <View>
                                        <Text style={styles.drinkName}>
                                            {item.quantity}x {item.drink_name}
                                        </Text>
                                        <Text style={styles.historyDesc}>{item.description}</Text>
                                    </View>
                                    <Text style={styles.historyDate}>{item.datetime}</Text>
                                </View>
                            ))}
                        </View>
                    )}
                </View>
            )}
        </ScrollView>
    );
}

const styles = StyleSheet.create({
    container: { flex: 1, backgroundColor: COLORS.background },
    scrollContent: {
        padding: 20,
        paddingTop: Platform.OS === "ios" ? 60 : 28,
        maxWidth: 760,
        marginHorizontal: "auto",
        width: "100%",
        paddingBottom: 50,
    },
    center: { flex: 1, justifyContent: "center", alignItems: "center", backgroundColor: COLORS.background, padding: 24 },
    loadingText: { marginTop: 12, color: COLORS.primary, fontSize: 16, fontWeight: "500" },
    errorTitle: { color: COLORS.danger, fontWeight: "700", fontSize: 18, marginTop: 12, textAlign: "center" },
    errorSub: { color: COLORS.textMuted, marginTop: 8, textAlign: "center" },

    statusBar: { flexDirection: "row", justifyContent: "space-between", alignItems: "center", marginBottom: 16 },
    statusBadge: { flexDirection: "row", alignItems: "center", gap: 8, backgroundColor: COLORS.white, paddingHorizontal: 12, paddingVertical: 6, borderRadius: 20, borderWidth: 1, borderColor: COLORS.border },
    statusDot: { width: 8, height: 8, borderRadius: 4 },
    statusText: { fontSize: 12, fontWeight: "600", color: COLORS.textDark },
    refreshBtn: { flexDirection: "row", alignItems: "center", gap: 4, padding: 6 },
    refreshText: { fontSize: 12, fontWeight: "600", color: COLORS.primary },

    toast: { flexDirection: "row", alignItems: "center", gap: 10, padding: 14, borderRadius: 14, marginBottom: 16 },
    toastSuccess: { backgroundColor: COLORS.successBg, borderWidth: 1, borderColor: COLORS.success },
    toastError: { backgroundColor: COLORS.dangerBg, borderWidth: 1, borderColor: COLORS.danger },
    toastText: { fontSize: 14, fontWeight: "600", flex: 1 },

    header: { flexDirection: "row", justifyContent: "space-between", alignItems: "center", marginBottom: 20 },
    greeting: { fontSize: 14, color: COLORS.textMuted, fontWeight: "500" },
    headerName: { fontSize: 26, fontWeight: "800", color: COLORS.textDark },
    headerActions: { flexDirection: "row", gap: 10 },
    profileIconBtn: { backgroundColor: COLORS.secondary, padding: 11, borderRadius: 50 },

    card: {
        backgroundColor: COLORS.white,
        borderRadius: 20,
        padding: 22,
        marginBottom: 20,
        shadowColor: COLORS.textDark,
        shadowOffset: { width: 0, height: 4 },
        shadowOpacity: 0.05,
        shadowRadius: 10,
        elevation: 3,
    },
    cardHeader: { flexDirection: "row", alignItems: "center", marginBottom: 6 },
    cardTitle: { fontSize: 13, fontWeight: "700", color: COLORS.textMuted, marginLeft: 8, textTransform: "uppercase", letterSpacing: 1 },
    balance: { fontSize: 40, fontWeight: "800", color: COLORS.textDark },
    currency: { fontSize: 22, color: COLORS.textMuted },
    balanceLow: { color: COLORS.danger },
    subSectionTitle: { fontSize: 13, fontWeight: "600", color: COLORS.textMuted, marginTop: 18, marginBottom: 10 },
    topUpRow: { flexDirection: "row", gap: 10 },
    topUpBtn: { flex: 1, backgroundColor: COLORS.background, paddingVertical: 11, borderRadius: 12, alignItems: "center", borderWidth: 1, borderColor: COLORS.secondary },
    topUpBtnText: { color: COLORS.primary, fontWeight: "700", fontSize: 15 },
    customTopUpRow: { flexDirection: "row", gap: 10, marginTop: 12, alignItems: "center" },

    editCard: { backgroundColor: COLORS.secondary },
    inputLabel: { fontSize: 14, color: COLORS.textDark, fontWeight: "700", marginBottom: 8 },
    input: { backgroundColor: COLORS.white, borderRadius: 12, padding: 12, marginBottom: 14, fontSize: 15, color: COLORS.textDark },
    editActions: { flexDirection: "row", justifyContent: "flex-end", gap: 10 },
    btnPrimary: { backgroundColor: COLORS.primary, paddingVertical: 12, paddingHorizontal: 18, borderRadius: 12, alignItems: "center", justifyContent: "center" },
    btnPrimaryText: { color: COLORS.white, fontWeight: "700", fontSize: 14 },
    btnSecondary: { paddingVertical: 12, paddingHorizontal: 16, borderRadius: 12, alignItems: "center" },
    btnSecondaryText: { color: COLORS.textDark, fontWeight: "600" },

    userChipsRow: { flexDirection: "row", marginBottom: 6 },
    userChip: { backgroundColor: COLORS.white, paddingHorizontal: 12, paddingVertical: 8, borderRadius: 20, marginRight: 8 },
    userChipActive: { backgroundColor: COLORS.primary },
    userChipText: { fontSize: 13, fontWeight: "600", color: COLORS.textDark },
    userChipTextActive: { color: COLORS.white },
    createUserRow: { flexDirection: "row", gap: 8, alignItems: "center" },

    tabsRow: { flexDirection: "row", gap: 10, marginBottom: 16 },
    tabBtn: {
        flex: 1,
        flexDirection: "row",
        alignItems: "center",
        justifyContent: "center",
        gap: 8,
        paddingVertical: 12,
        borderRadius: 14,
        backgroundColor: COLORS.white,
        borderWidth: 1,
        borderColor: COLORS.secondary,
    },
    tabBtnActive: { backgroundColor: COLORS.primary, borderColor: COLORS.primary },
    tabText: { fontWeight: "700", fontSize: 14, color: COLORS.primary },
    tabTextActive: { color: COLORS.white },

    searchBox: {
        flexDirection: "row",
        alignItems: "center",
        gap: 10,
        backgroundColor: COLORS.white,
        borderRadius: 14,
        paddingHorizontal: 14,
        paddingVertical: 10,
        marginBottom: 12,
        borderWidth: 1,
        borderColor: COLORS.border,
    },
    searchInput: { flex: 1, fontSize: 15, color: COLORS.textDark },
    catRow: { flexDirection: "row", marginBottom: 14 },
    catChip: { backgroundColor: COLORS.white, paddingHorizontal: 14, paddingVertical: 8, borderRadius: 20, marginRight: 8, borderWidth: 1, borderColor: COLORS.border },
    catChipActive: { backgroundColor: COLORS.primary, borderColor: COLORS.primary },
    catChipText: { fontSize: 13, fontWeight: "600", color: COLORS.textDark },
    catChipTextActive: { color: COLORS.white },

    sectionTitle: { fontSize: 20, fontWeight: "700", color: COLORS.textDark },
    drinksContainer: { gap: 10 },
    drinkCard: {
        flexDirection: "row",
        justifyContent: "space-between",
        alignItems: "center",
        backgroundColor: COLORS.white,
        padding: 15,
        borderRadius: 16,
        shadowColor: COLORS.textDark,
        shadowOffset: { width: 0, height: 2 },
        shadowOpacity: 0.03,
        shadowRadius: 8,
        elevation: 2,
    },
    drinkInfo: { flexDirection: "row", alignItems: "center", flex: 1 },
    drinkIconContainer: { backgroundColor: COLORS.background, padding: 10, borderRadius: 12, marginRight: 14 },
    drinkName: { fontSize: 16, fontWeight: "700", color: COLORS.textDark, marginBottom: 4 },
    drinkMetaRow: { flexDirection: "row", alignItems: "center", gap: 10, flexWrap: "wrap" },
    drinkPrice: { fontSize: 15, color: COLORS.primaryLight, fontWeight: "700" },
    drinkCat: { fontSize: 12, color: COLORS.textMuted, backgroundColor: COLORS.background, paddingHorizontal: 8, paddingVertical: 2, borderRadius: 8 },
    drinkStock: { fontSize: 12, color: COLORS.success, fontWeight: "600" },
    orderBtn: { backgroundColor: COLORS.primary, width: 42, height: 42, justifyContent: "center", alignItems: "center", borderRadius: 21 },

    historyHeader: { flexDirection: "row", justifyContent: "space-between", alignItems: "center", marginBottom: 12 },
    xmlBtn: { flexDirection: "row", alignItems: "center", gap: 6, backgroundColor: COLORS.secondary, paddingHorizontal: 12, paddingVertical: 8, borderRadius: 10 },
    xmlBtnText: { fontSize: 13, fontWeight: "700", color: COLORS.primary },
    xmlBox: { backgroundColor: "#2B211E", borderRadius: 14, padding: 14, marginBottom: 14 },
    xmlBoxHeader: { flexDirection: "row", justifyContent: "space-between", alignItems: "center", marginBottom: 8 },
    xmlBoxTitle: { color: "#E6D5C3", fontWeight: "700", fontSize: 13 },
    xmlCode: { color: "#FDF8F5", fontFamily: Platform.OS === "web" ? "monospace" : undefined, fontSize: 12 },
    historyCard: { flexDirection: "row", justifyContent: "space-between", alignItems: "center", backgroundColor: COLORS.white, padding: 15, borderRadius: 14 },
    historyDesc: { fontSize: 13, color: COLORS.textMuted, marginTop: 2 },
    historyDate: { fontSize: 12, color: COLORS.textMuted, fontWeight: "500" },

    emptyBox: { backgroundColor: COLORS.white, padding: 24, borderRadius: 16, alignItems: "center" },
    emptyText: { color: COLORS.textMuted, fontSize: 14 },
});
