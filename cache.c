/*
 * Copyright (c) 2008 Michael Kuhn
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "maki.h"

struct maki_cache_item
{
	gint ref_count;
	gpointer key;
	gpointer value;
};

static struct maki_cache_item* maki_cache_item_new (gpointer key, gpointer value)
{
	struct maki_cache_item* m_citem;

	m_citem = g_new(struct maki_cache_item, 1);
	m_citem->ref_count = 1;
	m_citem->key = key;
	m_citem->value = value;

	return m_citem;
}

static void maki_cache_item_free (struct maki_cache_item* m_citem)
{
	g_free(m_citem);
}

struct maki_cache* maki_cache_new (gpointer (*value_new) (gpointer, gpointer), void (*value_free) (gpointer), gpointer value_data)
{
	struct maki_cache* m_cache;

	g_return_val_if_fail(value_new != NULL, NULL);
	g_return_val_if_fail(value_free != NULL, NULL);

	m_cache = g_new(struct maki_cache, 1);
	m_cache->value_new = value_new;
	m_cache->value_free = value_free;
	m_cache->value_data = value_data;
	m_cache->hash_table = g_hash_table_new_full(maki_str_hash, maki_str_equal, NULL, NULL);

	return m_cache;
}

void maki_cache_free (struct maki_cache* cache)
{
	g_return_if_fail(cache != NULL);

	g_hash_table_destroy(cache->hash_table);
	g_free(cache);
}

gpointer maki_cache_insert (struct maki_cache* cache, gpointer key)
{
	gpointer value;
	struct maki_cache_item* m_citem;

	g_return_val_if_fail(cache != NULL, NULL);

	if ((m_citem = g_hash_table_lookup(cache->hash_table, key)) != NULL)
	{
		m_citem->ref_count++;

		return m_citem->value;
	}
	else
	{
		key = g_strdup(key);
		value = (*cache->value_new)(key, cache->value_data);

		m_citem = maki_cache_item_new(key, value);

		g_hash_table_insert(cache->hash_table, key, m_citem);

		return value;
	}
}

void maki_cache_remove (struct maki_cache* cache, gpointer key)
{
	struct maki_cache_item* m_citem;

	g_return_if_fail(cache != NULL);

	if ((m_citem = g_hash_table_lookup(cache->hash_table, key)) != NULL)
	{
		m_citem->ref_count--;

		if (m_citem->ref_count == 0)
		{
			g_hash_table_remove(cache->hash_table, m_citem->key);
			(*cache->value_free)(m_citem->value);
			g_free(m_citem->key);
			maki_cache_item_free(m_citem);
		}
	}
}
