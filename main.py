
@app.post("/cartao/verificar")
def verificar_cartao(data: VerificarCartaoRequest):
    """Verifica se o cartão está cadastrado e se está bloqueado."""

    uid = re.sub(r"[\s:-]", "", data.uid_cartao).upper()

    if uid.startswith("0X"):
        uid = uid[2:]

    if not uid or not re.fullmatch(r"[0-9A-F]+", uid):
        raise HTTPException(
            status_code=422,
            detail="UID do cartão inválido"
        )

    conn = get_conn()
    cur = conn.cursor()

    try:
        cur.execute(
            """
            SELECT id, cartao_bloqueado
            FROM usuarios
            WHERE REPLACE(
                REPLACE(
                    REPLACE(
                        REPLACE(UPPER(uid_cartao), ' ', ''),
                        ':', ''
                    ),
                    '-', ''
                ),
                '0X', ''
            ) = %s
            LIMIT 1
            """,
            (uid,),
        )

        usuario = cur.fetchone()

        # Cartão não cadastrado
        if usuario is None:
            return {
                "cadastrado": False,
                "cartao_bloqueado": None,
                "autorizado": False,
                "mensagem": "Cartao nao cadastrado"
            }

        # Dados encontrados no banco
        usuario_id, bloqueado = usuario
        bloqueado = bool(bloqueado)

        # Cartão cadastrado, mas bloqueado
        if bloqueado:
            return {
                "cadastrado": True,
                "cartao_bloqueado": True,
                "autorizado": False,
                "mensagem": "Cartao bloqueado"
            }

        # Cartão cadastrado e liberado
        return {
            "cadastrado": True,
            "cartao_bloqueado": False,
            "autorizado": True,
            "mensagem": "Cartao liberado"
        }

    finally:
        cur.close()
        conn.close()