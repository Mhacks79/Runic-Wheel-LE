# Rapport de validation Runic Wheel LE v0.17

**Statut** : version privée, compilation Win32/x86 réussie. Non testée dans Skyrim à ce stade.

Validation hors jeu : tests V015/V016/V017, tests D3D9 et textures RWAT, contrôles de typage LIGH, sérialisation RWL2 v2, suite de non-régression des actions natives héritées, signatures TESV LE 1.9.32.0.8, ABI et SEH MSVC, exports SKSE.

**Exigences utilisateur** : inversement main gauche/droite par clic en monde (implémenté), AZERTY A/E automatique (implémenté), molette des objets empilés (logique corrigée, validation jeu à faire), torche equip/desequip (route ajoutée, validation jeu à faire), icônes plus variées (32 nouveaux médaillons, tests d'isolation OK), compteur position/total (implémenté).

**Limite non résolue** : ingrédients. Le jeu distingue l'usage des ingrédients de la consommation ALCH. En l'absence de preuve de l'ABI/mécanique native, leur utilisation reste volontairement refusée, comme dans la v0.16. Aucune fausse promesse de prise en charge.

À tester dans une sauvegarde dédiée : clic G/D sur sort / objet équipable, position A/E clavier AZERTY, deux objets distincts dans un secteur puis molette, torche `LIGH` dans main gauche, ENB et co-save. Pour la molette, `EnableInputDiagnostics=1` donne un événement `input wheel pulse -> stack cursor stepped` dans le journal si reçu.
