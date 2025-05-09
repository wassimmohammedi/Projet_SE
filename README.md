🚍 Projet : Synchronisation des Bus de Transport dans un Tunnel 🛤️
🎓 Réalisé par :
-Mohammedi wassim          G2
-Loutfi belabbas mohamed   G2
-Mimouni nassim            G2






1. Règles Respectées par le Code
Pas de croisement :
Le tunnel a une seule direction à la fois (X ou Y). Les bus de la direction opposée attendent avec sem_wait.

Circulation groupée :
Plusieurs bus de la même ville peuvent entrer (count_x/count_y).

Équité :
La variable turn alterne entre X et Y pour éviter qu’une ville monopolise le tunnel.

10 allers-retours :
Chaque thread (bus) fait 10 boucles avec enter → sleep → exit dans les deux sens.

2. Exclusion Mutuelle (Vérification sur Papier)
Exclusion : Le mutex tunnel.mutex protège les accès aux variables.

Attente limitée : turn et les sémaphores garantissent que tous les bus passent.

Progrès : Les sémaphores réveillent les bus dès que possible.

Non-occupation : Les threads ne bloquent pas pendant les sleep.

3. Solution Valide ?
Oui, car :

Aucun croisement possible.

Aucune famine (grâce à turn).

Simple et vérifiable.
Limite : Peut être lent si une ville a beaucoup plus de bus que l’autre.

Réponse Ultra-Courte (pour un oral)
 Le code utilise un mutex pour protéger la direction du tunnel et des sémaphores pour faire attendre les bus si besoin. La variable turn alterne l’accès pour être équitable. Les 4 conditions de Coffman sont respectées, donc c’est une solution valide. 